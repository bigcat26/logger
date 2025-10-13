#ifndef __LOGGER_H__
#define __LOGGER_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdarg.h>
#include <stdint.h>
#include <string.h>

/* 注意：logger_marcos.h 需要单独包含以避免循环依赖 */
#include "logger/osal.h"

/* Configuration macros */
#define LOGGER_SINGLE_APPENDER   0
#define LOGGER_DEFAULT_LINE_SIZE 1024
#define LOGGER_MAX_APPENDERS      16
#define LOGGER_MAX_FILTERS        8

/* Log levels enumeration - 与 logger_marcos.h 中的常量保持一致 */
typedef enum {
  LOG_VERBOSE = 0,  /**< Verbose level - most detailed logging */
  LOG_DEBUG,         /**< Debug level - debugging information */
  LOG_INFO,          /**< Info level - general information */
  LOG_WARN,          /**< Warning level - warning messages */
  LOG_ERROR,         /**< Error level - error messages */
  LOG_FATAL          /**< Fatal level - critical errors */
} log_level_t;

/* Log event structure containing all log information */
struct logger_event_t {
  log_level_t level;       /**< Log level */
  uint32_t line;          /**< Source code line number */
  uint64_t timestamp_ns;  /**< Timestamp in nanoseconds */
  const char *file;       /**< Source file path */
  const char *func;       /**< Function name */
  uint16_t tid;           /**< Thread ID */
  uint16_t tag_id;        /**< Tag ID for categorization */
  uint16_t msg_len;       /**< Message length */
  uint16_t bin_len;       /**< Binary data length */
  const char *data;       /**< Log message data */
};

/* Forward declarations */
typedef struct logger_layout_t logger_layout_t;
typedef struct logger_filter_t logger_filter_t;
typedef struct logger_appender_t logger_appender_t;
typedef struct logger_t logger_t;

/* Layout function pointer type */
typedef int (*logger_layout_format_fn)(logger_layout_t *layout, 
                                       const struct logger_event_t *event, 
                                       char *buffer, 
                                       int buffer_size);

/* Layout structure for formatting log events */
struct logger_layout_t {
  logger_layout_format_fn format;  /**< Formatting function */
  void *user_data;                 /**< User-specific data */
};

/* Filter function pointer type */
typedef int (*logger_filter_fn)(logger_filter_t *filter, 
                                struct logger_event_t *event, 
                                void *user_data);

/* Filter structure for log filtering */
struct logger_filter_t {
  logger_filter_t *next;           /**< Next filter in chain */
  logger_filter_fn accept;        /**< Filter function */
  void *user_data;                 /**< User-specific data */
};

/* Appender write function pointer type */
typedef int (*logger_appender_write_fn)(logger_appender_t *appender, 
                                        const struct logger_event_t *event);

/* Appender structure for log output destinations */
struct logger_appender_t {
  logger_lock_t *lock;             /**< Thread synchronization lock */
  logger_appender_t *next;         /**< Next appender in chain */
  logger_layout_t *layout;         /**< Associated layout */
  logger_filter_t *filters;        /**< Filter chain */
  logger_appender_write_fn write;  /**< Write function */
  char *buffer;                    /**< Internal buffer */
  size_t buffer_size;              /**< Buffer size */
  void *user_data;                 /**< User-specific data */
};

/* Main logger structure */
struct logger_t {
#ifndef LOGGER_SINGLE_APPENDER
  logger_lock_t *lock;             /**< Thread synchronization lock */
#endif
  logger_appender_t *appenders;    /**< Appender chain */
  log_level_t min_level;           /**< Minimum log level */
  void *user_data;                 /**< User-specific data */
};

/* Configuration structures */

/* Logger configuration structure */
typedef struct {
  char *buffer;                    /**< Buffer for internal use */
  size_t buffer_size;              /**< Buffer size */
  log_level_t min_level;           /**< Minimum log level */
  int enable_thread_safety;        /**< Enable thread safety */
} logger_config_t;

/* Console appender configuration */
typedef struct {
  int enable_colors;               /**< Enable colored output */
  int enable_timestamp;            /**< Enable timestamp in output */
} logger_appender_console_config_t;

/* File appender configuration */
typedef struct {
  const char *filename;              /**< Log file path */
  size_t max_file_size;              /**< Maximum file size before rotation */
  int max_files;                      /**< Maximum number of rotated files */
  int enable_async;                   /**< Enable asynchronous writing */
  int flush_interval_ms;              /**< Flush interval in milliseconds */
  int rotation_mode;                  /**< Rotation mode: 0=size-based, 1=time-based */
  int rotation_interval_hours;        /**< Time-based rotation interval in hours */
  int enable_compression;             /**< Enable compression of rotated files */
  const char *backup_path;            /**< Custom backup path for rotated files */
} logger_appender_file_config_t;

/* Syslog appender configuration */
typedef struct {
  const char *host;                /**< Syslog server host */
  int port;                        /**< Syslog server port */
  const char *facility;            /**< Syslog facility */
  int enable_async;                /**< Enable asynchronous writing */
} logger_appender_syslog_config_t;

/* UDP appender configuration */
typedef struct {
  const char *host;                /**< UDP server host */
  int port;                        /**< UDP server port */
  int enable_async;                /**< Enable asynchronous writing */
} logger_appender_udp_config_t;

/* Core Logger API */

/**
 * Initialize logger with configuration
 * @param logger Logger instance to initialize
 * @param config Configuration parameters
 * @return 0 on success, negative value on error
 */
int logger_init(logger_t *logger, const logger_config_t *config);

/**
 * Deinitialize logger and free resources
 * @param logger Logger instance to deinitialize
 * @return 0 on success, negative value on error
 */
int logger_deinit(logger_t *logger);

/**
 * Quick startup - create and configure default logger
 * This function provides out-of-the-box logging capability
 * @return 0 on success, negative value on error
 */
int logger_quick_startup(void);

/**
 * Quick cleanup - destroy default logger
 * @return 0 on success, negative value on error
 */
int logger_quick_cleanup(void);

/**
 * Set default logger instance
 * @param logger Logger instance to set as default
 */
void logger_set_default_logger(logger_t *logger);

/**
 * Get default logger instance
 * @return Default logger instance, NULL if not initialized
 */
logger_t *logger_get_default_logger(void);

/* Appender Management API */

/**
 * Add appender to logger
 * @param logger Logger instance
 * @param appender Appender to add
 * @return 0 on success, negative value on error
 */
int logger_add_appender(logger_t *logger, logger_appender_t *appender);

/**
 * Remove appender from logger
 * @param logger Logger instance
 * @param appender Appender to remove
 * @return 0 on success, negative value on error
 */
int logger_remove_appender(logger_t *logger, logger_appender_t *appender);

/**
 * Create console appender
 * @param config Console appender configuration
 * @return Created appender instance, NULL on error
 */
logger_appender_t *logger_appender_create_console(const logger_appender_console_config_t *config);

/**
 * Create file appender
 * @param config File appender configuration
 * @return Created appender instance, NULL on error
 */
logger_appender_t *logger_appender_create_file(const logger_appender_file_config_t *config);

/**
 * Create syslog appender
 * @param config Syslog appender configuration
 * @return Created appender instance, NULL on error
 */
logger_appender_t *logger_appender_create_syslog(const logger_appender_syslog_config_t *config);

/**
 * Create UDP appender
 * @param config UDP appender configuration
 * @return Created appender instance, NULL on error
 */
logger_appender_t *logger_appender_create_udp(const logger_appender_udp_config_t *config);

/**
 * Destroy appender and free resources
 * @param appender Appender to destroy
 * @return 0 on success, negative value on error
 */
int logger_appender_destroy(logger_appender_t *appender);

/* Layout Management API */

/**
 * Create full layout (includes timestamp, level, file, line, function, thread ID)
 * @return Created layout instance, NULL on error
 */
logger_layout_t *logger_layout_create_full(void);

/**
 * Create classic layout (includes timestamp, level, thread ID)
 * @return Created layout instance, NULL on error
 */
logger_layout_t *logger_layout_create_classic(void);

/**
 * Create simple layout (minimal information)
 * @return Created layout instance, NULL on error
 */
logger_layout_t *logger_layout_create_simple(void);

/**
 * Create syslog layout (RFC 3164 compliant)
 * @return Created layout instance, NULL on error
 */
logger_layout_t *logger_layout_create_syslog(void);

/**
 * Set layout for appender
 * @param appender Appender instance
 * @param layout Layout to set
 * @return 0 on success, negative value on error
 */
int logger_appender_set_layout(logger_appender_t *appender, logger_layout_t *layout);

/**
 * Destroy layout and free resources
 * @param layout Layout to destroy
 * @return 0 on success, negative value on error
 */
int logger_layout_destroy(logger_layout_t *layout);

/* Filter Management API */

/**
 * Create level filter
 * @param min_level Minimum log level to accept
 * @return Created filter instance, NULL on error
 */
logger_filter_t *logger_filter_create_level(log_level_t min_level);

/**
 * Create custom filter
 * @param filter_fn Custom filter function
 * @param user_data User data passed to filter function
 * @return Created filter instance, NULL on error
 */
logger_filter_t *logger_filter_create_custom(logger_filter_fn filter_fn, void *user_data);

/**
 * Add filter to appender
 * @param appender Appender instance
 * @param filter Filter to add
 * @return 0 on success, negative value on error
 */
int logger_appender_add_filter(logger_appender_t *appender, logger_filter_t *filter);

/**
 * Remove filter from appender
 * @param appender Appender instance
 * @param filter Filter to remove
 * @return 0 on success, negative value on error
 */
int logger_appender_remove_filter(logger_appender_t *appender, logger_filter_t *filter);

/**
 * Destroy filter and free resources
 * @param filter Filter to destroy
 * @return 0 on success, negative value on error
 */
int logger_filter_destroy(logger_filter_t *filter);

/* Logging API */

/**
 * Log formatted message with file and line information
 * @param logger Logger instance (NULL for default logger)
 * @param level Log level
 * @param file Source file name
 * @param line Source line number
 * @param func Function name
 * @param format Format string
 * @param ... Format arguments
 * @return 0 on success, negative value on error
 */
int logger_printf(logger_t *logger, log_level_t level, const char *file, 
                  unsigned int line, const char *func, const char *format, ...);

/**
 * Log formatted message without file/line information
 * @param logger Logger instance (NULL for default logger)
 * @param level Log level
 * @param format Format string
 * @param ... Format arguments
 * @return 0 on success, negative value on error
 */
int logger_catf(logger_t *logger, log_level_t level, const char *format, ...);

/**
 * Log binary data
 * @param logger Logger instance (NULL for default logger)
 * @param level Log level
 * @param file Source file name
 * @param line Source line number
 * @param func Function name
 * @param data Binary data
 * @param len Data length
 * @return 0 on success, negative value on error
 */
int logger_printb(logger_t *logger, log_level_t level, const char *file,
                  unsigned int line, const char *func, const void *data, size_t len);

/* Utility API */

/**
 * Set minimum log level for logger
 * @param logger Logger instance
 * @param level Minimum log level
 * @return 0 on success, negative value on error
 */
int logger_set_level(logger_t *logger, log_level_t level);

/**
 * Get minimum log level for logger
 * @param logger Logger instance
 * @return Current minimum log level
 */
log_level_t logger_get_level(logger_t *logger);

/**
 * Flush all appenders
 * @param logger Logger instance
 * @return 0 on success, negative value on error
 */
int logger_flush(logger_t *logger);

/**
 * Get log level name as string
 * @param level Log level
 * @return Log level name string
 */
const char *logger_level_to_string(log_level_t level);

/**
 * Parse log level from string
 * @param str Log level string
 * @return Log level, LOG_INFO if parsing fails
 */
log_level_t logger_string_to_level(const char *str);

/* ============================================================================
 * LOG MACROS - 日志宏定义
 * ============================================================================ */

#define LOG_LEVEL_MASK           (0x00000007)
#define LOG_LEVEL_ALL            (0xFFFFFFFF)

#if defined(__UCOS__) || defined(__FH8610__) || defined(cc3200)
#define LOGGER_ENDL                 "\r\n"
#else
#define LOGGER_ENDL                 "\n"
#endif

#define LOGGER_NONE                 "\e[0m"
#define LOGGER_BLACK                "\e[0;30m"
#define LOGGER_L_BLACK              "\e[1;30m"
#define LOGGER_RED                  "\e[0;31m"
#define LOGGER_L_RED                "\e[1;31m"
#define LOGGER_GREEN                "\e[0;32m"
#define LOGGER_L_GREEN              "\e[1;32m"
#define LOGGER_BROWN                "\e[0;33m"
#define LOGGER_YELLOW               "\e[1;33m"
#define LOGGER_BLUE                 "\e[0;34m"
#define LOGGER_L_BLUE               "\e[1;34m"
#define LOGGER_PURPLE               "\e[0;35m"
#define LOGGER_L_PURPLE             "\e[1;35m"
#define LOGGER_CYAN                 "\e[0;36m"
#define LOGGER_L_CYAN               "\e[1;36m"
#define LOGGER_GRAY                 "\e[0;37m"
#define LOGGER_WHITE                "\e[1;37m"

#define LOGGER_BOLD                 "\e[1m"
#define LOGGER_UNDERLINE            "\e[4m"
#define LOGGER_BLINK                "\e[5m"
#define LOGGER_REVERSE              "\e[7m"
#define LOGGER_HIDE                 "\e[8m"
#define LOGGER_CLEAR                "\e[2J"
#define LOGGER_CLRLINE              "\r\e[K"

/* 基础宏定义 - 支持不同编译器 */
#if defined(_MSC_VER)
#define LOG(level, fmt, ...)        logger_printf(logger_get_default_logger(), level, __FILE__, __LINE__, __FUNCTION__, fmt, __VA_ARGS__)
#define LOGL(level, fmt, ...)       logger_printf(logger_get_default_logger(), level, __FILE__, __LINE__, __FUNCTION__, fmt LOGGER_ENDL, __VA_ARGS__)
#define LOGC(level, fmt, ...)       logger_catf(logger_get_default_logger(), level, fmt, __VA_ARGS__)
#define LOGT(level, tag, fmt, ...)  logger_printf(logger_get_default_logger(), level, __FILE__, __LINE__, __FUNCTION__, "[" tag "] " fmt, __VA_ARGS__)
#define LOGTL(level, tag, fmt, ...) logger_printf(logger_get_default_logger(), level, __FILE__, __LINE__, __FUNCTION__, "[" tag "] " fmt LOGGER_ENDL, __VA_ARGS__)
#define LOGTC(level, tag, fmt, ...) logger_catf(logger_get_default_logger(), level, "[" tag "] " fmt, __VA_ARGS__)
#else
#define LOG(level, fmt, args...)    logger_printf(logger_get_default_logger(), level, __FILE__, __LINE__, __FUNCTION__, fmt, ##args)
#define LOGL(level, fmt, args...)  logger_printf(logger_get_default_logger(), level, __FILE__, __LINE__, __FUNCTION__, fmt LOGGER_ENDL, ##args)
#define LOGC(level, fmt, args...)  logger_catf(logger_get_default_logger(), level, fmt, ##args)
#define LOGT(level, tag, fmt, args...) logger_printf(logger_get_default_logger(), level, __FILE__, __LINE__, __FUNCTION__, "[" tag "] " fmt, ##args)
#define LOGTL(level, tag, fmt, args...) logger_printf(logger_get_default_logger(), level, __FILE__, __LINE__, __FUNCTION__, "[" tag "] " fmt LOGGER_ENDL, ##args)
#define LOGTC(level, tag, fmt, args...) logger_catf(logger_get_default_logger(), level, "[" tag "] " fmt, ##args)
#endif

/* 二进制数据日志宏 */
#define LOGB(level, dat, len)       logger_printb(logger_get_default_logger(), level, __FILE__, __LINE__, __FUNCTION__, dat, len)
#define LOGBT(level, tag, dat, len) logger_printb(logger_get_default_logger(), level, __FILE__, __LINE__, __FUNCTION__, dat, len)

/* 标准日志宏 */
#if defined(_MSC_VER)
#define LOGF(fmt, ...)              LOG(LOG_FATAL,    fmt, __VA_ARGS__)
#define LOGE(fmt, ...)              LOG(LOG_ERROR,    fmt, __VA_ARGS__)
#define LOGW(fmt, ...)              LOG(LOG_WARN,     fmt, __VA_ARGS__)
#define LOGI(fmt, ...)              LOG(LOG_INFO,     fmt, __VA_ARGS__)
#define LOGD(fmt, ...)              LOG(LOG_DEBUG,    fmt, __VA_ARGS__)
#define LOGV(fmt, ...)              LOG(LOG_VERBOSE,  fmt, __VA_ARGS__)
#define LOGLF(fmt, ...)             LOGL(LOG_FATAL,   fmt, __VA_ARGS__)
#define LOGLE(fmt, ...)             LOGL(LOG_ERROR,   fmt, __VA_ARGS__)
#define LOGLW(fmt, ...)             LOGL(LOG_WARN,    fmt, __VA_ARGS__)
#define LOGLI(fmt, ...)             LOGL(LOG_INFO,    fmt, __VA_ARGS__)
#define LOGLD(fmt, ...)             LOGL(LOG_DEBUG,   fmt, __VA_ARGS__)
#define LOGLV(fmt, ...)             LOGL(LOG_VERBOSE, fmt, __VA_ARGS__)
#define LOGCF(fmt, ...)             LOGC(LOG_FATAL,   fmt, __VA_ARGS__)
#define LOGCE(fmt, ...)             LOGC(LOG_ERROR,   fmt, __VA_ARGS__)
#define LOGCW(fmt, ...)             LOGC(LOG_WARN,    fmt, __VA_ARGS__)
#define LOGCI(fmt, ...)             LOGC(LOG_INFO,    fmt, __VA_ARGS__)
#define LOGCD(fmt, ...)             LOGC(LOG_DEBUG,   fmt, __VA_ARGS__)
#define LOGCV(fmt, ...)             LOGC(LOG_VERBOSE, fmt, __VA_ARGS__)

/* 带 tag 的日志宏 */
#define LOGTF(tag, fmt, ...)        LOGT(LOG_FATAL,   tag, fmt, __VA_ARGS__)
#define LOGTE(tag, fmt, ...)        LOGT(LOG_ERROR,   tag, fmt, __VA_ARGS__)
#define LOGTW(tag, fmt, ...)        LOGT(LOG_WARN,    tag, fmt, __VA_ARGS__)
#define LOGTI(tag, fmt, ...)        LOGT(LOG_INFO,    tag, fmt, __VA_ARGS__)
#define LOGTD(tag, fmt, ...)        LOGT(LOG_DEBUG,   tag, fmt, __VA_ARGS__)
#define LOGTV(tag, fmt, ...)        LOGT(LOG_VERBOSE, tag, fmt, __VA_ARGS__)
#define LOGTLF(tag, fmt, ...)       LOGTL(LOG_FATAL,  tag, fmt, __VA_ARGS__)
#define LOGTLE(tag, fmt, ...)       LOGTL(LOG_ERROR,  tag, fmt, __VA_ARGS__)
#define LOGTLW(tag, fmt, ...)       LOGTL(LOG_WARN,   tag, fmt, __VA_ARGS__)
#define LOGTLI(tag, fmt, ...)       LOGTL(LOG_INFO,   tag, fmt, __VA_ARGS__)
#define LOGTLD(tag, fmt, ...)       LOGTL(LOG_DEBUG,  tag, fmt, __VA_ARGS__)
#define LOGTLV(tag, fmt, ...)       LOGTL(LOG_VERBOSE,tag, fmt, __VA_ARGS__)
#define LOGTCF(tag, fmt, ...)       LOGTC(LOG_FATAL,  tag, fmt, __VA_ARGS__)
#define LOGTCE(tag, fmt, ...)       LOGTC(LOG_ERROR,  tag, fmt, __VA_ARGS__)
#define LOGTCW(tag, fmt, ...)       LOGTC(LOG_WARN,   tag, fmt, __VA_ARGS__)
#define LOGTCI(tag, fmt, ...)       LOGTC(LOG_INFO,   tag, fmt, __VA_ARGS__)
#define LOGTCD(tag, fmt, ...)       LOGTC(LOG_DEBUG,  tag, fmt, __VA_ARGS__)
#define LOGTCV(tag, fmt, ...)       LOGTC(LOG_VERBOSE,tag, fmt, __VA_ARGS__)

/* 二进制数据日志宏 */
#define LOGBF(dat, len)             LOGB(LOG_FATAL,   dat, len)
#define LOGBE(dat, len)             LOGB(LOG_ERROR,   dat, len)
#define LOGBW(dat, len)             LOGB(LOG_WARN,    dat, len)
#define LOGBI(dat, len)             LOGB(LOG_INFO,    dat, len)
#define LOGBD(dat, len)             LOGB(LOG_DEBUG,   dat, len)
#define LOGBV(dat, len)             LOGB(LOG_VERBOSE, dat, len)
#define LOGBTF(tag, dat, len)       LOGBT(LOG_FATAL,  tag, dat, len)
#define LOGBTE(tag, dat, len)       LOGBT(LOG_ERROR,  tag, dat, len)
#define LOGBTW(tag, dat, len)       LOGBT(LOG_WARN,   tag, dat, len)
#define LOGBTI(tag, dat, len)       LOGBT(LOG_INFO,   tag, dat, len)
#define LOGBTD(tag, dat, len)       LOGBT(LOG_DEBUG,  tag, dat, len)
#define LOGBTV(tag, dat, len)       LOGBT(LOG_VERBOSE,tag, dat, len)

#elif defined(__FH8610__) || defined(__UCOS__)

// workaround macros for hcac compiler
#define _LOGF(fmt, args...)         LOG(LOG_FATAL,    fmt, ##args)
#define _LOGE(fmt, args...)         LOG(LOG_ERROR,    fmt, ##args)
#define _LOGW(fmt, args...)         LOG(LOG_WARN,     fmt, ##args)
#define _LOGI(fmt, args...)         LOG(LOG_INFO,     fmt, ##args)
#define _LOGD(fmt, args...)         LOG(LOG_DEBUG,    fmt, ##args)
#define _LOGV(fmt, args...)         LOG(LOG_VERBOSE,  fmt, ##args)
#define _LOGLF(fmt, args...)        LOGL(LOG_FATAL,   fmt, ##args)
#define _LOGLE(fmt, args...)        LOGL(LOG_ERROR,   fmt, ##args)
#define _LOGLW(fmt, args...)        LOGL(LOG_WARN,    fmt, ##args)
#define _LOGLI(fmt, args...)        LOGL(LOG_INFO,    fmt, ##args)
#define _LOGLD(fmt, args...)        LOGL(LOG_DEBUG,   fmt, ##args)
#define _LOGLV(fmt, args...)        LOGL(LOG_VERBOSE, fmt, ##args)
#define _LOGCF(fmt, args...)        LOGC(LOG_FATAL,   fmt, ##args)
#define _LOGCE(fmt, args...)        LOGC(LOG_ERROR,   fmt, ##args)
#define _LOGCW(fmt, args...)        LOGC(LOG_WARN,    fmt, ##args)
#define _LOGCI(fmt, args...)        LOGC(LOG_INFO,    fmt, ##args)
#define _LOGCD(fmt, args...)        LOGC(LOG_DEBUG,   fmt, ##args)
#define _LOGCV(fmt, args...)        LOGC(LOG_VERBOSE, fmt, ##args)

#define LOGF(args...)               _LOGF(##args, "")
#define LOGE(args...)               _LOGE(##args, "")
#define LOGW(args...)               _LOGW(##args, "")
#define LOGI(args...)               _LOGI(##args, "")
#define LOGD(args...)               _LOGD(##args, "")
#define LOGV(args...)               _LOGV(##args, "")
#define LOGLF(args...)              _LOGLF(##args, "")
#define LOGLE(args...)              _LOGLE(##args, "")
#define LOGLW(args...)              _LOGLW(##args, "")
#define LOGLI(args...)              _LOGLI(##args, "")
#define LOGLD(args...)              _LOGLD(##args, "")
#define LOGLV(args...)              _LOGLV(##args, "")
#define LOGCF(args...)              _LOGCF(##args, "")
#define LOGCE(args...)              _LOGCE(##args, "")
#define LOGCW(args...)              _LOGCW(##args, "")
#define LOGCI(args...)              _LOGCI(##args, "")
#define LOGCD(args...)              _LOGCD(##args, "")
#define LOGCV(args...)              _LOGCV(##args, "")

/* 带 tag 的日志宏 */
#define LOGTF(tag, fmt, args...)    LOGT(LOG_FATAL,   tag, fmt, ##args)
#define LOGTE(tag, fmt, args...)    LOGT(LOG_ERROR,   tag, fmt, ##args)
#define LOGTW(tag, fmt, args...)    LOGT(LOG_WARN,    tag, fmt, ##args)
#define LOGTI(tag, fmt, args...)    LOGT(LOG_INFO,    tag, fmt, ##args)
#define LOGTD(tag, fmt, args...)    LOGT(LOG_DEBUG,   tag, fmt, ##args)
#define LOGTV(tag, fmt, args...)    LOGT(LOG_VERBOSE, tag, fmt, ##args)
#define LOGTLF(tag, fmt, args...)   LOGTL(LOG_FATAL,  tag, fmt, ##args)
#define LOGTLE(tag, fmt, args...)   LOGTL(LOG_ERROR,  tag, fmt, ##args)
#define LOGTLW(tag, fmt, args...)   LOGTL(LOG_WARN,   tag, fmt, ##args)
#define LOGTLI(tag, fmt, args...)   LOGTL(LOG_INFO,   tag, fmt, ##args)
#define LOGTLD(tag, fmt, args...)   LOGTL(LOG_DEBUG,  tag, fmt, ##args)
#define LOGTLV(tag, fmt, args...)   LOGTL(LOG_VERBOSE,tag, fmt, ##args)
#define LOGTCF(tag, fmt, args...)   LOGTC(LOG_FATAL,  tag, fmt, ##args)
#define LOGTCE(tag, fmt, args...)   LOGTC(LOG_ERROR,  tag, fmt, ##args)
#define LOGTCW(tag, fmt, args...)   LOGTC(LOG_WARN,   tag, fmt, ##args)
#define LOGTCI(tag, fmt, args...)   LOGTC(LOG_INFO,   tag, fmt, ##args)
#define LOGTCD(tag, fmt, args...)   LOGTC(LOG_DEBUG,  tag, fmt, ##args)
#define LOGTCV(tag, fmt, args...)   LOGTC(LOG_VERBOSE,tag, fmt, ##args)

/* 二进制数据日志宏 */
#define LOGBF(dat, len)             LOGB(LOG_FATAL,   dat, len)
#define LOGBE(dat, len)             LOGB(LOG_ERROR,   dat, len)
#define LOGBW(dat, len)             LOGB(LOG_WARN,    dat, len)
#define LOGBI(dat, len)             LOGB(LOG_INFO,    dat, len)
#define LOGBD(dat, len)             LOGB(LOG_DEBUG,   dat, len)
#define LOGBV(dat, len)             LOGB(LOG_VERBOSE, dat, len)
#define LOGBTF(tag, dat, len)       LOGBT(LOG_FATAL,  tag, dat, len)
#define LOGBTE(tag, dat, len)       LOGBT(LOG_ERROR,  tag, dat, len)
#define LOGBTW(tag, dat, len)       LOGBT(LOG_WARN,   tag, dat, len)
#define LOGBTI(tag, dat, len)       LOGBT(LOG_INFO,   tag, dat, len)
#define LOGBTD(tag, dat, len)       LOGBT(LOG_DEBUG,  tag, dat, len)
#define LOGBTV(tag, dat, len)       LOGBT(LOG_VERBOSE,tag, dat, len)

#else // GCC/Clang

#define LOGF(fmt, args...)          LOG(LOG_FATAL,    fmt, ##args)
#define LOGE(fmt, args...)          LOG(LOG_ERROR,    fmt, ##args)
#define LOGW(fmt, args...)          LOG(LOG_WARN,     fmt, ##args)
#define LOGI(fmt, args...)          LOG(LOG_INFO,     fmt, ##args)
#define LOGD(fmt, args...)          LOG(LOG_DEBUG,    fmt, ##args)
#define LOGV(fmt, args...)          LOG(LOG_VERBOSE,  fmt, ##args)
#define LOGLF(fmt, args...)         LOGL(LOG_FATAL,   fmt, ##args)
#define LOGLE(fmt, args...)         LOGL(LOG_ERROR,   fmt, ##args)
#define LOGLW(fmt, args...)         LOGL(LOG_WARN,    fmt, ##args)
#define LOGLI(fmt, args...)         LOGL(LOG_INFO,    fmt, ##args)
#define LOGLD(fmt, args...)         LOGL(LOG_DEBUG,   fmt, ##args)
#define LOGLV(fmt, args...)         LOGL(LOG_VERBOSE, fmt, ##args)
#define LOGCF(fmt, args...)         LOGC(LOG_FATAL,   fmt, ##args)
#define LOGCE(fmt, args...)         LOGC(LOG_ERROR,   fmt, ##args)
#define LOGCW(fmt, args...)         LOGC(LOG_WARN,    fmt, ##args)
#define LOGCI(fmt, args...)         LOGC(LOG_INFO,    fmt, ##args)
#define LOGCD(fmt, args...)         LOGC(LOG_DEBUG,   fmt, ##args)
#define LOGCV(fmt, args...)         LOGC(LOG_VERBOSE, fmt, ##args)

/* 带 tag 的日志宏 */
#define LOGTF(tag, fmt, args...)    LOGT(LOG_FATAL,   tag, fmt, ##args)
#define LOGTE(tag, fmt, args...)    LOGT(LOG_ERROR,   tag, fmt, ##args)
#define LOGTW(tag, fmt, args...)    LOGT(LOG_WARN,    tag, fmt, ##args)
#define LOGTI(tag, fmt, args...)    LOGT(LOG_INFO,    tag, fmt, ##args)
#define LOGTD(tag, fmt, args...)    LOGT(LOG_DEBUG,   tag, fmt, ##args)
#define LOGTV(tag, fmt, args...)    LOGT(LOG_VERBOSE, tag, fmt, ##args)
#define LOGTLF(tag, fmt, args...)   LOGTL(LOG_FATAL,  tag, fmt, ##args)
#define LOGTLE(tag, fmt, args...)   LOGTL(LOG_ERROR,  tag, fmt, ##args)
#define LOGTLW(tag, fmt, args...)   LOGTL(LOG_WARN,   tag, fmt, ##args)
#define LOGTLI(tag, fmt, args...)   LOGTL(LOG_INFO,   tag, fmt, ##args)
#define LOGTLD(tag, fmt, args...)   LOGTL(LOG_DEBUG,  tag, fmt, ##args)
#define LOGTLV(tag, fmt, args...)   LOGTL(LOG_VERBOSE,tag, fmt, ##args)
#define LOGTCF(tag, fmt, args...)   LOGTC(LOG_FATAL,  tag, fmt, ##args)
#define LOGTCE(tag, fmt, args...)   LOGTC(LOG_ERROR,  tag, fmt, ##args)
#define LOGTCW(tag, fmt, args...)   LOGTC(LOG_WARN,   tag, fmt, ##args)
#define LOGTCI(tag, fmt, args...)   LOGTC(LOG_INFO,   tag, fmt, ##args)
#define LOGTCD(tag, fmt, args...)   LOGTC(LOG_DEBUG,  tag, fmt, ##args)
#define LOGTCV(tag, fmt, args...)   LOGTC(LOG_VERBOSE,tag, fmt, ##args)

/* 二进制数据日志宏 */
#define LOGBF(dat, len)             LOGB(LOG_FATAL,   dat, len)
#define LOGBE(dat, len)             LOGB(LOG_ERROR,   dat, len)
#define LOGBW(dat, len)             LOGB(LOG_WARN,    dat, len)
#define LOGBI(dat, len)             LOGB(LOG_INFO,    dat, len)
#define LOGBD(dat, len)             LOGB(LOG_DEBUG,   dat, len)
#define LOGBV(dat, len)             LOGB(LOG_VERBOSE, dat, len)
#define LOGBTF(tag, dat, len)       LOGBT(LOG_FATAL,  tag, dat, len)
#define LOGBTE(tag, dat, len)       LOGBT(LOG_ERROR,  tag, dat, len)
#define LOGBTW(tag, dat, len)       LOGBT(LOG_WARN,   tag, dat, len)
#define LOGBTI(tag, dat, len)       LOGBT(LOG_INFO,   tag, dat, len)
#define LOGBTD(tag, dat, len)       LOGBT(LOG_DEBUG,  tag, dat, len)
#define LOGBTV(tag, dat, len)       LOGBT(LOG_VERBOSE,tag, dat, len)

#endif // 编译器选择

#ifdef __cplusplus
}
#endif

#endif // __LOGGER_H__
