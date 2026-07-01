#ifndef HTTP_H
#define HTTP_H

typedef struct {
    char method[16];
    char path[256];
    char version[16];
} http_request_t;

// Parses an HTTP request line, e.g. "GET / HTTP/1.1", out of raw_request
// (which may have trailing headers/body after it -- only the first line
// is used). Returns 0 on success, -1 if it doesn't contain exactly a
// method, path, and version.
int http_parse_request_line(const char *raw_request, http_request_t *out);

// Returns 1 if path is safe to resolve underneath the served directory
// (i.e. contains no ".." traversal component), 0 otherwise.
int http_path_is_safe(const char *path);

#endif
