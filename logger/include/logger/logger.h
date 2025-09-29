#ifndef __LOGGER_H__
#define __LOGGER_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdarg.h>
#include <stdint.h>
#include <string.h>


#include "logger/logger_marcos.h"
#include "logger/osal.h"

#define LOGGER_SINGLE_APPENDER   0
#define LOGGER_DEFAULT_LINE_SIZE 1024
// typedef void (*LOGGER_WRITER)(struct logger_appender_t *appender, int level,
// const char *buf, int len);

typedef enum {
  LOG_LEVEL_VERBOSE,
  LOG_LEVEL_DEBUG,
  LOG_LEVEL_INFO,
  LOG_LEVEL_WARN,
  LOG_LEVEL_ERROR,
  LOG_LEVEL_FATAL
} log_level_t;

struct logger_event_t {
  log_level_t level;
  uint32_t line;
  uint64_t timestamp_ns;
  const char *file;
  const char *func;
  uint16_t tid;
  uint16_t tag_id;
  uint16_t msg_len;
  uint16_t bin_len;
  const char *data;
};

struct logger_layout_t {
  int (*format)(struct logger_layout_t *, const struct logger_event_t *, char *, int);
};

struct logger_filter_t {
  struct logger_filter_t *next;
  int (*accept)(struct logger_filter_t *filter, struct logger_event_t *);
};

struct logger_appender_t {
  logger_lock_t *lock;
  struct logger_appender_t *next;
  struct logger_layout_t *layout;
  struct logger_filter_t *filters;
  int (*write)(struct logger_appender_t *, const struct logger_event_t *);
  char *buf;
  size_t buf_size;
};

struct logger_t {
#ifndef LOGGER_SINGLE_APPENDER
  logger_lock_t *lock;
#endif
  struct logger_appender_t *appenders;
};

// void logger_printb(struct logger_t *logger, int level, const char *file,
// unsigned int line, const void *buf, int len);

// void logger_printf(struct logger_t *logger, int level, const char *file,
// unsigned int line, const char *fmt, ...);

// void logger_catf(struct logger_t *logger, int level, const char *fmt, ...);

// void logger_layout_add_appender(struct logger_layout_t *layout, struct
// logger_appender_t *appender);

// void logger_add_layout(struct logger_t *logger, struct logger_layout_t
// *layout);

// void logger_set_level_mask(struct logger_t *logger, int level_mask);

void logger_add_appender(struct logger_t *logger,
                         struct logger_appender_t *appender);

void logger_remove_appender(struct logger_t *logger,
                            struct logger_appender_t *appender);

void logger_set_default_logger(struct logger_t *logger);

struct logger_t *logger_get_default_logger();

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
