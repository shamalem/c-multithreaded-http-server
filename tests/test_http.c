#include "http.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static void test_parse_valid_get(void) {
    http_request_t req;
    int rc = http_parse_request_line("GET / HTTP/1.1\r\nHost: x\r\n\r\n", &req);
    assert(rc == 0);
    assert(strcmp(req.method, "GET") == 0);
    assert(strcmp(req.path, "/") == 0);
    assert(strcmp(req.version, "HTTP/1.1") == 0);
    printf("PASS test_parse_valid_get\n");
}

static void test_parse_valid_post_with_path(void) {
    http_request_t req;
    int rc = http_parse_request_line("POST /submit HTTP/1.0\r\n\r\n", &req);
    assert(rc == 0);
    assert(strcmp(req.method, "POST") == 0);
    assert(strcmp(req.path, "/submit") == 0);
    assert(strcmp(req.version, "HTTP/1.0") == 0);
    printf("PASS test_parse_valid_post_with_path\n");
}

static void test_parse_malformed_missing_fields(void) {
    http_request_t req;
    int rc = http_parse_request_line("GARBAGE\r\n\r\n", &req);
    assert(rc == -1);
    printf("PASS test_parse_malformed_missing_fields\n");
}

static void test_parse_empty_request(void) {
    http_request_t req;
    int rc = http_parse_request_line("", &req);
    assert(rc == -1);
    printf("PASS test_parse_empty_request\n");
}

static void test_path_traversal_rejected(void) {
    assert(http_path_is_safe("/../etc/passwd") == 0);
    assert(http_path_is_safe("/foo/../../etc/passwd") == 0);
    printf("PASS test_path_traversal_rejected\n");
}

static void test_normal_paths_accepted(void) {
    assert(http_path_is_safe("/") == 1);
    assert(http_path_is_safe("/index.html") == 1);
    assert(http_path_is_safe("/css/style.css") == 1);
    printf("PASS test_normal_paths_accepted\n");
}

int main(void) {
    test_parse_valid_get();
    test_parse_valid_post_with_path();
    test_parse_malformed_missing_fields();
    test_parse_empty_request();
    test_path_traversal_rejected();
    test_normal_paths_accepted();
    printf("All tests passed.\n");
    return 0;
}
