#include "thread_pool.h"
#include "request_handler.h"
#include "log.h"

#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// One queued connection waiting for a worker to pick it up.
typedef struct task {
    int conn_fd;
    struct task *next;
} task_t;

struct thread_pool {
    pthread_t *threads;
    int num_threads;

    task_t *queue_head;
    task_t *queue_tail;

    pthread_mutex_t lock;
    pthread_cond_t not_empty;
    int shutdown;
};

static void *worker_main(void *arg) {
    thread_pool_t *pool = arg;

    while (1) {
        pthread_mutex_lock(&pool->lock);
        while (pool->queue_head == NULL && !pool->shutdown) {
            pthread_cond_wait(&pool->not_empty, &pool->lock);
        }

        if (pool->queue_head == NULL && pool->shutdown) {
            pthread_mutex_unlock(&pool->lock);
            break;
        }

        task_t *task = pool->queue_head;
        pool->queue_head = task->next;
        if (pool->queue_head == NULL) {
            pool->queue_tail = NULL;
        }
        pthread_mutex_unlock(&pool->lock);

        handle_client(task->conn_fd);
        free(task);
    }

    return NULL;
}

thread_pool_t *thread_pool_create(int num_threads) {
    thread_pool_t *pool = malloc(sizeof(thread_pool_t));
    if (pool == NULL) {
        log_msg(LOG_ERROR, "thread_pool_create: out of memory");
        exit(EXIT_FAILURE);
    }
    pool->num_threads = num_threads;
    pool->queue_head = NULL;
    pool->queue_tail = NULL;
    pool->shutdown = 0;
    pthread_mutex_init(&pool->lock, NULL);
    pthread_cond_init(&pool->not_empty, NULL);

    pool->threads = malloc(sizeof(pthread_t) * (size_t)num_threads);
    if (pool->threads == NULL) {
        log_msg(LOG_ERROR, "thread_pool_create: out of memory");
        exit(EXIT_FAILURE);
    }
    for (int i = 0; i < num_threads; i++) {
        int rc = pthread_create(&pool->threads[i], NULL, worker_main, pool);
        if (rc != 0) {
            log_msg(LOG_ERROR, "pthread_create: %s", strerror(rc));
            exit(EXIT_FAILURE);
        }
    }
    log_msg(LOG_INFO, "Thread pool started with %d workers", num_threads);
    return pool;
}

void thread_pool_submit(thread_pool_t *pool, int conn_fd) {
    task_t *task = malloc(sizeof(task_t));
    if (task == NULL) {
        log_msg(LOG_ERROR, "thread_pool_submit: out of memory, dropping fd=%d", conn_fd);
        close(conn_fd);
        return;
    }
    task->conn_fd = conn_fd;
    task->next = NULL;

    pthread_mutex_lock(&pool->lock);
    if (pool->queue_tail == NULL) {
        pool->queue_head = task;
    } else {
        pool->queue_tail->next = task;
    }
    pool->queue_tail = task;
    pthread_cond_signal(&pool->not_empty);
    pthread_mutex_unlock(&pool->lock);
}

void thread_pool_destroy(thread_pool_t *pool) {
    pthread_mutex_lock(&pool->lock);
    pool->shutdown = 1;
    pthread_cond_broadcast(&pool->not_empty);
    pthread_mutex_unlock(&pool->lock);

    for (int i = 0; i < pool->num_threads; i++) {
        pthread_join(pool->threads[i], NULL);
    }

    task_t *task = pool->queue_head;
    while (task != NULL) {
        task_t *next = task->next;
        close(task->conn_fd);
        free(task);
        task = next;
    }

    pthread_mutex_destroy(&pool->lock);
    pthread_cond_destroy(&pool->not_empty);
    free(pool->threads);
    free(pool);
    log_msg(LOG_INFO, "Thread pool shut down cleanly");
}
