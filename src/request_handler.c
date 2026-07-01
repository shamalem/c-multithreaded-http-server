#include "request_handler.h"
#include "http.h"
#include "log.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

void handle_client(int conn_fd) {
    log_msg(LOG_INFO, "Client connected: fd=%d", conn_fd);

    char buffer[4096];
    ssize_t bytes_read = read(conn_fd, buffer, sizeof(buffer) - 1);
    if (bytes_read == -1) {
        log_msg(LOG_WARN, "read: %s", strerror(errno));
        close(conn_fd);
        return;
    }
    buffer[bytes_read] = '\0';

    http_request_t req;
    if (http_parse_request_line(buffer, &req) != 0) {
        log_msg(LOG_WARN, "Malformed request line");
        const char *bad_request =
            "HTTP/1.1 400 Bad Request\r\n"
            "Content-Type: text/plain\r\n"
            "Content-Length: 11\r\n"
            "\r\n"
            "Bad Request";
        write(conn_fd, bad_request, strlen(bad_request));
        close(conn_fd);
        return;
    }
    log_msg(LOG_INFO, "%s %s %s", req.method, req.path, req.version);

    if (strcmp(req.path, "/slow") == 0) {
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
        return;
    }

    if (!http_path_is_safe(req.path)) {
        log_msg(LOG_WARN, "Rejected path traversal attempt: %s", req.path);
        const char *not_found =
            "HTTP/1.1 404 Not Found\r\n"
            "Content-Type: text/plain\r\n"
            "Content-Length: 9\r\n"
            "\r\n"
            "Not Found";
        write(conn_fd, not_found, strlen(not_found));
        close(conn_fd);
        return;
    }

    char full_path[512];
    if (strcmp(req.path, "/") == 0) {
        snprintf(full_path, sizeof(full_path), "public/index.html");
    } else {
        snprintf(full_path, sizeof(full_path), "public%s", req.path);
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
        return;
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
        return;
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
