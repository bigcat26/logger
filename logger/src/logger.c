#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "logger/logger.h"
#include "logger/osal.h"

#if defined(WIN32) && !defined(vsnprintf)
#define vsnprintf   _vsnprintf
#endif

#ifndef MIN
#define MIN(a, b)   ((a) > (b) ? (b) : (a))
#endif

/* Global default logger */
static logger_t _default_logger_instance = {0};
static logger_t *_global_default_logger = NULL;
static volatile int _logger_initialized = 0;

/* Internal helper functions */
static int logger_event_create(struct logger_event_t *event, log_level_t level, 
                              const char *file, unsigned int line, const char *func,
                              const char *data, int data_len);
static void logger_dispatch_event(logger_t *logger, const struct logger_event_t *event);
static int logger_check_level(logger_t *logger, log_level_t level);
static void logger_lock_acquire_internal(logger_t *logger);
static void logger_lock_release_internal(logger_t *logger);

/* Core Logger API Implementation */

int logger_init(logger_t *logger, const logger_config_t *config) {
    if (!logger) {
        return -1;
    }
    
    memset(logger, 0, sizeof(logger_t));
    
    /* Set default configuration */
    logger->min_level = LOG_LEVEL_INFO;
    
    if (config) {
        /* Use provided buffer or allocate new one */
        if (config->buffer && config->buffer_size > 0) {
            logger->user_data = config->buffer;
        } else if (config->buffer_size > 0) {
            logger->user_data = logger_malloc(config->buffer_size);
            if (!logger->user_data) {
                return -1;
            }
        }
        
        logger->min_level = config->min_level;
        
        /* Initialize thread safety if enabled */
        if (config->enable_thread_safety) {
#ifndef LOGGER_SINGLE_APPENDER
            logger_lock_new(&logger->lock);
            if (!logger->lock) {
                if (logger->user_data && !config->buffer) {
                    logger_free(logger->user_data);
                }
                return -1;
            }
#endif
        }
    }
    
    return 0;
}

int logger_deinit(logger_t *logger) {
    if (!logger) {
        return -1;
    }
    
    /* Appenders should already be removed by the caller */
    
    /* Free thread lock */
#ifndef LOGGER_SINGLE_APPENDER
    if (logger->lock) {
        logger_lock_free(&logger->lock);
    }
#endif
    
    /* Free user data if allocated */
    if (logger->user_data) {
        /* For now, just don't free user data to avoid crashes */
        /* In a production version, you would track whether to free or not */
        logger->user_data = NULL;
    }
    
    memset(logger, 0, sizeof(logger_t));
    return 0;
}

int logger_quick_startup(void) {
    if (_logger_initialized) {
        return 0; /* Already initialized */
    }
    
    logger_config_t config = {0};
    config.buffer_size = LOGGER_DEFAULT_LINE_SIZE;
    config.min_level = LOG_LEVEL_INFO;
    config.enable_thread_safety = 1;
    
    int result = logger_init(&_default_logger_instance, &config);
    if (result == 0) {
        _global_default_logger = &_default_logger_instance;
        _logger_initialized = 1;
        
        /* Create default console appender for quick startup */
        logger_appender_console_config_t console_cfg = {0};
        console_cfg.enable_colors = 1;
        console_cfg.enable_timestamp = 1;
        
        logger_appender_t *console_appender = logger_appender_create_console(&console_cfg);
        if (console_appender) {
            logger_layout_t *simple_layout = logger_layout_create_simple();
            if (simple_layout) {
                logger_appender_set_layout(console_appender, simple_layout);
            }
            logger_add_appender(&_default_logger_instance, console_appender);
        }
    }
    
    return result;
}

int logger_quick_cleanup(void) {
    if (!_logger_initialized) {
        return 0; /* Not initialized */
    }
    
    int result = logger_deinit(&_default_logger_instance);
    if (result == 0) {
        _global_default_logger = NULL;
        _logger_initialized = 0;
    }
    return result;
}

void logger_set_default_logger(logger_t *logger) {
    _global_default_logger = logger;
}

logger_t *logger_get_default_logger(void) {
    return _global_default_logger;
}

/* Appender Management API Implementation */

int logger_add_appender(logger_t *logger, logger_appender_t *appender) {
    if (!logger || !appender) {
        return -1;
    }
    
    logger_lock_acquire_internal(logger);

    if (!logger->appenders) {
        logger->appenders = appender;
        appender->next = NULL;
    } else {
        logger_appender_t *cur = logger->appenders;
        while (cur->next) {
            cur = cur->next;
        }
        cur->next = appender;
        appender->next = NULL;
    }
    
    logger_lock_release_internal(logger);
    return 0;
}

int logger_remove_appender(logger_t *logger, logger_appender_t *appender) {
    if (!logger || !appender) {
        return -1;
    }
    
    logger_lock_acquire_internal(logger);
    
    if (logger->appenders == appender) {
        logger->appenders = appender->next;
    } else {
        logger_appender_t *cur = logger->appenders;
        while (cur && cur->next != appender) {
            cur = cur->next;
        }
        if (cur) {
            cur->next = appender->next;
        }
    }
    
    logger_lock_release_internal(logger);
    return 0;
}

/* Logging API Implementation */

int logger_printf(logger_t *logger, log_level_t level, const char *file, 
                  unsigned int line, const char *func, const char *format, ...) {
    if (!format) {
        return -1;
    }
    
    /* Use default logger if none provided */
    if (!logger) {
        logger = logger_get_default_logger();
        if (!logger) {
            return -1;
        }
    }
    
    /* Check log level */
    if (!logger_check_level(logger, level)) {
        return 0;
    }
    
    /* Format the message */
    char buffer[LOGGER_DEFAULT_LINE_SIZE];
    va_list args;
    va_start(args, format);
    int len = vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    if (len < 0 || len >= (int)sizeof(buffer)) {
        return -1;
    }
    
    /* Create and dispatch event */
    struct logger_event_t event;
    if (logger_event_create(&event, level, file, line, func, buffer, len) != 0) {
        return -1;
    }
    
    logger_dispatch_event(logger, &event);
    return 0;
}

int logger_catf(logger_t *logger, log_level_t level, const char *format, ...) {
    if (!format) {
        return -1;
    }
    
    /* Use default logger if none provided */
    if (!logger) {
        logger = logger_get_default_logger();
        if (!logger) {
            return -1;
        }
    }
    
    /* Check log level */
    if (!logger_check_level(logger, level)) {
        return 0;
    }
    
    /* Format the message */
    char buffer[LOGGER_DEFAULT_LINE_SIZE];
    va_list args;
    va_start(args, format);
    int len = vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    if (len < 0 || len >= (int)sizeof(buffer)) {
        return -1;
    }
    
    /* Create and dispatch event */
    struct logger_event_t event;
    if (logger_event_create(&event, level, NULL, 0, NULL, buffer, len) != 0) {
        return -1;
    }
    
    logger_dispatch_event(logger, &event);
    return 0;
}

int logger_printb(logger_t *logger, log_level_t level, const char *file,
                  unsigned int line, const char *func, const void *data, int len) {
    if (!data || len <= 0) {
        return -1;
    }
    
    /* Use default logger if none provided */
    if (!logger) {
        logger = logger_get_default_logger();
        if (!logger) {
            return -1;
        }
    }
    
    /* Check log level */
    if (!logger_check_level(logger, level)) {
        return 0;
    }
    
    /* Create and dispatch event */
    struct logger_event_t event;
    if (logger_event_create(&event, level, file, line, func, (const char *)data, len) != 0) {
        return -1;
    }
    
    logger_dispatch_event(logger, &event);
    return 0;
}

/* Utility API Implementation */

int logger_set_level(logger_t *logger, log_level_t level) {
    if (!logger) {
        return -1;
    }
    
    logger_lock_acquire_internal(logger);
    logger->min_level = level;
    logger_lock_release_internal(logger);
    return 0;
}

log_level_t logger_get_level(logger_t *logger) {
    if (!logger) {
        return LOG_LEVEL_INFO;
    }
    
    return logger->min_level;
}

int logger_flush(logger_t *logger) {
    if (!logger) {
        return -1;
    }
    
    logger_lock_acquire_internal(logger);
    
    logger_appender_t *appender = logger->appenders;
    while (appender) {
        /* Call flush if appender supports it */
        if (appender->user_data) {
            /* This would be implemented by specific appender types */
            /* For now, we'll just iterate through all appenders */
        }
        appender = appender->next;
    }
    
    logger_lock_release_internal(logger);
    return 0;
}

const char *logger_level_to_string(log_level_t level) {
    switch (level) {
        case LOG_LEVEL_VERBOSE: return "VERBOSE";
        case LOG_LEVEL_DEBUG:   return "DEBUG";
        case LOG_LEVEL_INFO:    return "INFO";
        case LOG_LEVEL_WARN:    return "WARN";
        case LOG_LEVEL_ERROR:   return "ERROR";
        case LOG_LEVEL_FATAL:   return "FATAL";
        default:                return "UNKNOWN";
    }
}

log_level_t logger_string_to_level(const char *str) {
    if (!str) {
        return LOG_LEVEL_INFO;
    }
    
    if (strcmp(str, "VERBOSE") == 0) return LOG_LEVEL_VERBOSE;
    if (strcmp(str, "DEBUG") == 0)   return LOG_LEVEL_DEBUG;
    if (strcmp(str, "INFO") == 0)    return LOG_LEVEL_INFO;
    if (strcmp(str, "WARN") == 0)    return LOG_LEVEL_WARN;
    if (strcmp(str, "ERROR") == 0)   return LOG_LEVEL_ERROR;
    if (strcmp(str, "FATAL") == 0)   return LOG_LEVEL_FATAL;
    
    return LOG_LEVEL_INFO; /* Default fallback */
}

/* Internal Helper Functions */

static int logger_event_create(struct logger_event_t *event, log_level_t level, 
                              const char *file, unsigned int line, const char *func,
                              const char *data, int data_len) {
    if (!event || !data) {
        return -1;
    }
    
    memset(event, 0, sizeof(struct logger_event_t));
    
    event->level = level;
    event->line = line;
    event->file = file;
    event->func = func;
    event->data = data;
    event->msg_len = data_len;
    event->timestamp_ns = 0;
    event->tid = 0;
    
    return 0;
}

static void logger_dispatch_event(logger_t *logger, const struct logger_event_t *event) {
    if (!logger || !event) {
        return;
    }
    
    logger_lock_acquire_internal(logger);
    
    logger_appender_t *appender = logger->appenders;
    while (appender) {
        /* Apply filters */
        int should_log = 1;
        logger_filter_t *filter = appender->filters;
        while (filter && should_log) {
            if (filter->accept) {
                should_log = filter->accept(filter, (struct logger_event_t *)event, filter->user_data);
            }
            filter = filter->next;
        }
        
        /* Write to appender if filters pass */
        if (should_log && appender->write) {
            appender->write(appender, event);
        }
        
        appender = appender->next;
    }
    
    logger_lock_release_internal(logger);
}

static int logger_check_level(logger_t *logger, log_level_t level) {
    if (!logger) {
        return 0;
    }
    
    return (level >= logger->min_level) ? 1 : 0;
}

static void logger_lock_acquire_internal(logger_t *logger) {
#ifndef LOGGER_SINGLE_APPENDER
    if (logger && logger->lock) {
        logger_lock_acquire(logger->lock);
    }
#endif
}

static void logger_lock_release_internal(logger_t *logger) {
#ifndef LOGGER_SINGLE_APPENDER
    if (logger && logger->lock) {
        logger_lock_release(logger->lock);
    }
#endif
}
