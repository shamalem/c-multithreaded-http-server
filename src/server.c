#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>

int main(void) {
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }
    printf("Socket created: fd=%d\n", listen_fd);

    int opt = 1;
    if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(8080);

    if (bind(listen_fd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        perror("bind");
        exit(EXIT_FAILURE);
    }
    printf("Bound to port 8080\n");

    if (listen(listen_fd, 10) == -1) {
        perror("listen");
        exit(EXIT_FAILURE);
    }
    printf("Listening on port 8080...\n");

    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    while(1){
        int conn_fd = accept(listen_fd, (struct sockaddr *)&client_addr, &client_len);
        if (conn_fd == -1) {
            perror("accept");
            continue;
        }

        printf("Client connected!\n");

        char buffer[4096];
        ssize_t bytes_read = read(conn_fd, buffer, sizeof(buffer) - 1);
        if (bytes_read == -1) {
            perror("read");
            close(conn_fd);
            continue;
        }
        buffer[bytes_read] = '\0';
        printf("--- Request ---\n%s\n----------------\n", buffer);

        char method[16];
        char path[256];
        char version[16];
        int matched = sscanf(buffer, "%15s %255s %15s", method, path, version);
        if (matched != 3) {
            printf("Malformed request line\n");
            close(conn_fd);
            continue;
        }
        printf("method=%s path=%s version=%s\n", method, path, version);

        const char *not_found =
            "HTTP/1.1 404 Not Found\r\n"
            "Content-Type: text/plain\r\n"
            "Content-Length: 9\r\n"
            "\r\n"
            "Not Found";

        if (strstr(path, "..") != NULL) {
            printf("Rejected path traversal attempt: %s\n", path);
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
            perror("open");
            write(conn_fd, not_found, strlen(not_found));
            close(conn_fd);
            continue;
        }

        struct stat st;
        fstat(file_fd, &st);

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
