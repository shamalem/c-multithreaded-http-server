#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "log.h"
#include "request_handler.h"

// Thread entry point: takes ownership of the heap-allocated fd, frees
// it, and hands the connection off to handle_client().
static void *client_thread(void *arg) {
    int conn_fd = *(int *)arg;
    free(arg);
    handle_client(conn_fd);
    return NULL;
}

int main(void) {
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

    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    while(1){
        int conn_fd = accept(listen_fd, (struct sockaddr *)&client_addr, &client_len);
        if (conn_fd == -1) {
            log_msg(LOG_WARN, "accept: %s", strerror(errno));
            continue;
        }

        int *fd_ptr = malloc(sizeof(int));
        if (fd_ptr == NULL) {
            log_msg(LOG_ERROR, "malloc failed, dropping connection fd=%d", conn_fd);
            close(conn_fd);
            continue;
        }
        *fd_ptr = conn_fd;

        pthread_t tid;
        int rc = pthread_create(&tid, NULL, client_thread, fd_ptr);
        if (rc != 0) {
            log_msg(LOG_ERROR, "pthread_create: %s", strerror(rc));
            free(fd_ptr);
            close(conn_fd);
            continue;
        }
        pthread_detach(tid);
    }
    close(listen_fd);
    return 0;
}
