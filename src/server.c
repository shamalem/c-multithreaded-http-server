#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>

#include "log.h"

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

        log_msg(LOG_INFO, "Client connected: fd=%d", conn_fd);

        char buffer[4096];
        ssize_t bytes_read = read(conn_fd, buffer, sizeof(buffer) - 1);
        if (bytes_read == -1) {
            log_msg(LOG_WARN, "read: %s", strerror(errno));
            close(conn_fd);
            continue;
        }
        buffer[bytes_read] = '\0';

        char method[16];
        char path[256];
        char version[16];
        int matched = sscanf(buffer, "%15s %255s %15s", method, path, version);
        if (matched != 3) {
            log_msg(LOG_WARN, "Malformed request line");
            const char *bad_request =
                "HTTP/1.1 400 Bad Request\r\n"
                "Content-Type: text/plain\r\n"
                "Content-Length: 11\r\n"
                "\r\n"
                "Bad Request";
            write(conn_fd, bad_request, strlen(bad_request));
            close(conn_fd);
            continue;
        }
        log_msg(LOG_INFO, "%s %s %s", method, path, version);

        if (strcmp(path, "/slow") == 0) {
            log_msg(LOG_INFO, "Handling /slow: sleeping 5s (fd=%d)", conn_fd);
            sleep(5);
            const char *slow_ok =
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: text/plain\r\n"
                "Content-Length: 14\r\n"
                "\r\n"
                "Slow response!";
            write(conn_fd, slow_ok, strlen(slow_ok));
            log_msg(LOG_INFO, "Finished /slow (fd=%d)", conn_fd);
            close(conn_fd);
            continue;
        }

        if (strstr(path, "..") != NULL) {
            log_msg(LOG_WARN, "Rejected path traversal attempt: %s", path);
            const char *not_found =
                "HTTP/1.1 404 Not Found\r\n"
                "Content-Type: text/plain\r\n"
                "Content-Length: 9\r\n"
                "\r\n"
                "Not Found";
            write(conn_fd, not_found, strlen(not_found));
            close(conn_fd);
            continue;
        }

        char full_path[512];
        if (strcmp(path, "/") == 0) {
            snprintf(full_path, sizeof(full_path), "public/index.html");
        } else {
            snprintf(full_path, sizeof(full_path), "public%s", path);
        }

        int file_fd = open(full_path, O_RDONLY);
        if (file_fd == -1) {
            if (errno == EACCES) {
                log_msg(LOG_WARN, "Forbidden: %s", full_path);
                const char *forbidden =
                    "HTTP/1.1 403 Forbidden\r\n"
                    "Content-Type: text/plain\r\n"
                    "Content-Length: 9\r\n"
                    "\r\n"
                    "Forbidden";
                write(conn_fd, forbidden, strlen(forbidden));
            } else {
                log_msg(LOG_WARN, "Not found: %s (%s)", full_path, strerror(errno));
                const char *not_found =
                    "HTTP/1.1 404 Not Found\r\n"
                    "Content-Type: text/plain\r\n"
                    "Content-Length: 9\r\n"
                    "\r\n"
                    "Not Found";
                write(conn_fd, not_found, strlen(not_found));
            }
            close(conn_fd);
            continue;
        }

        struct stat st;
        if (fstat(file_fd, &st) == -1) {
            log_msg(LOG_ERROR, "fstat: %s", strerror(errno));
            const char *server_error =
                "HTTP/1.1 500 Internal Server Error\r\n"
                "Content-Type: text/plain\r\n"
                "Content-Length: 21\r\n"
                "\r\n"
                "Internal Server Error";
            write(conn_fd, server_error, strlen(server_error));
            close(file_fd);
            close(conn_fd);
            continue;
        }

        char header[256];
        int header_len = snprintf(header, sizeof(header),
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/html\r\n"
            "Content-Length: %lld\r\n"
            "\r\n",
            (long long)st.st_size);
        write(conn_fd, header, header_len);

        char file_buf[4096];
        ssize_t n;
        while ((n = read(file_fd, file_buf, sizeof(file_buf))) > 0) {
            write(conn_fd, file_buf, n);
        }
        close(file_fd);

        close(conn_fd);
    }
    close(listen_fd);
    return 0;
}
