#ifndef THREAD_POOL_H
#define THREAD_POOL_H

typedef struct thread_pool thread_pool_t;

// Creates a pool of num_threads worker threads, ready to accept work.
thread_pool_t *thread_pool_create(int num_threads);

// Queues conn_fd for a worker to run handle_client() on. Thread-safe;
// may be called concurrently and from a different thread than created
// the pool.
void thread_pool_submit(thread_pool_t *pool, int conn_fd);

// Wakes all idle workers, joins every worker thread (waiting for any
// in-flight handle_client() call to finish), closes any fds still
// sitting in the queue, and frees the pool. Not safe to call
// concurrently with thread_pool_submit().
void thread_pool_destroy(thread_pool_t *pool);

#endif
