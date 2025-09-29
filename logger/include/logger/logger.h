#ifndef __LOGGER_H__
#define __LOGGER_H__

#include <stdarg.h>
#include <string.h>
#include <time.h>

#include "logger/osal.h"
#include "logger/logger_marcos.h"

#define LOGGER_DEFAULT_LINE_SIZE 1024

// struct logger_t;
// struct logger_appender_t;
// struct logger_layout_t;

// typedef void (*LOGGER_WRITER)(struct logger_appender_t *appender, int level, const char *buf, int len);

// struct logger_filter_t {
//     struct logger_filter_t *next;
//     int (*filter)(struct logger_filter_t *filter, int level, const char *buf, int len);
// }

typedef int (*LOGGER_FORMATTER_STR)(struct logger_layout_t *layout, char *buf, int n, int level, const char *file, unsigned int line, const char *fmt, va_list ap);
typedef int (*LOGGER_FORMATTER_BIN)(struct logger_layout_t *layout, char *buf, int n, int level, const char *file, unsigned int line, const void *dat, int len);

struct logger_layout_t {
    LOGGER_FORMATTER_STR format_str;
    LOGGER_FORMATTER_BIN format_bin;
};

struct logger_appender_t {
    struct logger_appender_t *next;
    struct logger_layout_t layout;
    struct logger_filter_t *filters;
    LOGGER_WRITER writer;
};

struct logger_t {
    char *buf;
    int buf_size;
    int own_buf;
    logger_lock_t *lock;
    struct logger_appender_t *appenders;
};

#ifdef __cplusplus
extern "C" {
#endif

// struct logger_t *logger_get_default();

// void logger_printb(struct logger_t *logger, int level, const char *file, unsigned int line, const void *buf, int len);

// void logger_printf(struct logger_t *logger, int level, const char *file, unsigned int line, const char *fmt, ...);

// void logger_catf(struct logger_t *logger, int level, const char *fmt, ...);

// void logger_layout_add_appender(struct logger_layout_t *layout, struct logger_appender_t *appender);

// void logger_add_layout(struct logger_t *logger, struct logger_layout_t *layout);

// void logger_set_level_mask(struct logger_t *logger, int level_mask);


// void logger_set_default(struct logger_t *logger);

// struct logger_t *logger_get_default();


int logger_init(struct logger_t *logger, char *buf, int buf_size);

int logger_deinit((struct logger_t *logger);

/**
 * create global default logger
 */
int logger_quick_startup(void);

int logger_quick_cleanup(void);

#ifdef __cplusplus
};
#endif

#endif // __LOGGER_H__
