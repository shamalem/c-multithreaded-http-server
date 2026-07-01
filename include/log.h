#ifndef LOG_H
#define LOG_H

typedef enum { LOG_INFO, LOG_WARN, LOG_ERROR } log_level_t;

void log_msg(log_level_t level, const char *fmt, ...) __attribute__((format(printf, 2, 3)));

#endif
