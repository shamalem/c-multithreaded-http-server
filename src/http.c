#include "http.h"

#include <stdio.h>
#include <string.h>

int http_parse_request_line(const char *raw_request, http_request_t *out) {
    int matched = sscanf(raw_request, "%15s %255s %15s",
                          out->method, out->path, out->version);
    return (matched == 3) ? 0 : -1;
}

int http_path_is_safe(const char *path) {
    return strstr(path, "..") == NULL;
}
