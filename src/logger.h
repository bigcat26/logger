#ifndef __LOGGER_H__
#define __LOGGER_H__

#include <time.h>
#include <stdarg.h>
#include <string.h>
#include "logger/logger_marcos.h"

#ifdef _MSC_VER
#define LOGGER_PATH_SLASH       '\\'
#else
#define LOGGER_PATH_SLASH       '/'
#endif

struct LOGGER;
struct LOGGER_APPENDER;
struct LOGGER_LAYOUT;

typedef void *logger_lock_t;
typedef time_t (*PFNLOGGER_TIME)(time_t *);
typedef struct tm *(*PFNLOGGER_LOCALTIME)(const time_t *);
typedef void (*PFNLOGGER_LOCK_ACQUIRE)(logger_lock_t);
typedef void (*PFNLOGGER_LOCK_RELEASE)(logger_lock_t);

struct LOGGER_CFG
{
    char *buf;
    int buf_size;
    logger_lock_t lock;
    PFNLOGGER_TIME syscall_time;
    PFNLOGGER_LOCALTIME syscall_localtime;
    PFNLOGGER_LOCK_ACQUIRE acquire_lock;
    PFNLOGGER_LOCK_RELEASE release_lock;
};

typedef void (*LOGGER_WRITER)(struct LOGGER_APPENDER *appender, int level, const char *buf, int len);

struct LOGGER_APPENDER
{
    struct LOGGER_APPENDER *next;
    int level_mask;
    LOGGER_WRITER writer;
};

typedef int (*LOGGER_FORMATTER_STR)(struct LOGGER_LAYOUT *layout, char *buf, int n, int level, const char *file, unsigned int line, const char *fmt, va_list ap);
typedef int (*LOGGER_FORMATTER_BIN)(struct LOGGER_LAYOUT *layout, char *buf, int n, int level, const char *file, unsigned int line, const void *dat, int len);

struct LOGGER_LAYOUT
{
    struct LOGGER *logger;
    struct LOGGER_LAYOUT *next;
    LOGGER_FORMATTER_STR format_str;
    LOGGER_FORMATTER_BIN format_bin;
    struct LOGGER_APPENDER *appenders;
};

struct LOGGER
{
    int level_mask;
    struct LOGGER_CFG cfg;
    struct LOGGER_LAYOUT *layouts;
};

#ifdef __cplusplus
extern "C" {
#endif

struct LOGGER *logger_get_default();

void logger_printb(struct LOGGER *logger, int level, const char *file, unsigned int line, const void *buf, int len);

void logger_printf(struct LOGGER *logger, int level, const char *file, unsigned int line, const char *fmt, ...);

void logger_catf(struct LOGGER *logger, int level, const char *fmt, ...);

void logger_layout_add_appender(struct LOGGER_LAYOUT *layout, struct LOGGER_APPENDER *appender);

void logger_add_layout(struct LOGGER *logger, struct LOGGER_LAYOUT *layout);

void logger_set_level_mask(struct LOGGER *logger, int level_mask);

void logger_init(struct LOGGER *logger, struct LOGGER_CFG *cfg);

void logger_set_default(struct LOGGER *logger);

struct LOGGER *logger_get_default();

#ifdef __cplusplus
};
#endif

#endif // __LOGGER_H__
