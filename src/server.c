#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "log.h"
#include "thread_pool.h"

#define NUM_WORKER_THREADS 4

// Signal handlers must do as little as possible: no malloc, no
// printf/log_msg (not async-signal-safe), no non-trivial logic. We
// only set a flag of a type guaranteed to be safely written from a
// signal handler, sig_atomic_t, marked volatile so the compiler
// doesn't cache it in a register and miss the update.
static volatile sig_atomic_t shutdown_requested = 0;

static void handle_shutdown_signal(int sig) {
    (void)sig;
    shutdown_requested = 1;
}

int main(void) {
    struct sigaction sa = {0};
    sa.sa_handler = handle_shutdown_signal;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0; // deliberately no SA_RESTART: we want accept() to
                      // return -1/EINTR so the loop notices the signal
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd == -1) {
        log_msg(LOG_ERROR, "socket: %s", strerror(errno));
        exit(EXIT_FAILURE);
    }
    log_msg(LOG_INFO, "Socket created: fd=%d", listen_fd);

    int opt = 1;
    if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        log_msg(LOG_ERROR, "setsockopt: %s", strerror(errno));
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(8080);

    if (bind(listen_fd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        log_msg(LOG_ERROR, "bind: %s", strerror(errno));
        exit(EXIT_FAILURE);
    }
    log_msg(LOG_INFO, "Bound to port 8080");

    if (listen(listen_fd, 10) == -1) {
        log_msg(LOG_ERROR, "listen: %s", strerror(errno));
        exit(EXIT_FAILURE);
    }
    log_msg(LOG_INFO, "Listening on port 8080...");

    thread_pool_t *pool = thread_pool_create(NUM_WORKER_THREADS);

    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    while (!shutdown_requested) {
        int conn_fd = accept(listen_fd, (struct sockaddr *)&client_addr, &client_len);
        if (conn_fd == -1) {
            if (errno == EINTR) {
                break; // interrupted by our signal handler; shutdown_requested is now set
            }
            log_msg(LOG_WARN, "accept: %s", strerror(errno));
            continue;
        }

        thread_pool_submit(pool, conn_fd);
    }

    log_msg(LOG_INFO, "Shutdown signal received, draining thread pool...");
    close(listen_fd);
    thread_pool_destroy(pool);
    log_msg(LOG_INFO, "Shutdown complete");
    return 0;
}
