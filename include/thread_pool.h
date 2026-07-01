#ifndef THREAD_POOL_H
#define THREAD_POOL_H

typedef struct thread_pool thread_pool_t;

// Creates a pool of num_threads worker threads, ready to accept work.
thread_pool_t *thread_pool_create(int num_threads);

// Queues conn_fd for a worker to run handle_client() on. Thread-safe;
// may be called concurrently and from a different thread than created
// the pool.
void thread_pool_submit(thread_pool_t *pool, int conn_fd);

#endif
