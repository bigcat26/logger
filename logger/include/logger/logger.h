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

/* Configuration macros */
#define LOGGER_SINGLE_APPENDER   0
#define LOGGER_DEFAULT_LINE_SIZE 1024
#define LOGGER_MAX_APPENDERS      16
#define LOGGER_MAX_FILTERS        8

/* Log levels enumeration */
typedef enum {
  LOG_LEVEL_VERBOSE = 0,  /**< Verbose level - most detailed logging */
  LOG_LEVEL_DEBUG,         /**< Debug level - debugging information */
  LOG_LEVEL_INFO,          /**< Info level - general information */
  LOG_LEVEL_WARN,          /**< Warning level - warning messages */
  LOG_LEVEL_ERROR,         /**< Error level - error messages */
  LOG_LEVEL_FATAL          /**< Fatal level - critical errors */
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
  const char *filename;            /**< Log file path */
  size_t max_file_size;            /**< Maximum file size before rotation */
  int max_files;                   /**< Maximum number of rotated files */
  int enable_async;                /**< Enable asynchronous writing */
  int flush_interval_ms;           /**< Flush interval in milliseconds */
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
                  unsigned int line, const char *func, const void *data, int len);

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
 * @return Log level, LOG_LEVEL_INFO if parsing fails
 */
log_level_t logger_string_to_level(const char *str);

#ifdef __cplusplus
}
#endif

#endif // __LOGGER_H__
