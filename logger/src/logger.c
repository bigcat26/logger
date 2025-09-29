#include <stdio.h>
#include "logger/logger.h"

#if defined(WIN32) && !defined(vsnprintf)
#define vsnprintf   _vsnprintf
#endif

#ifndef MIN
#define MIN(a, b)   ((a) > (b) ? (b) : (a))
#endif

// #define logger_lock(logger)          do { if ((logger)->cfg.acquire_lock) { (logger)->cfg.acquire_lock((logger)->cfg.lock); } } while(0)
// #define logger_unlock(logger)        do { if ((logger)->cfg.release_lock) { (logger)->cfg.release_lock((logger)->cfg.lock); } } while(0)

// #define logger_buffer(logger)        ((logger)->cfg.buf)
// #define logger_buffer_size(logger)   ((logger)->cfg.buf_size)

// global default logger
static struct logger_t _logger = {0};
static struct logger_t *_default_logger = &_logger;
static volatile int _logger_initialized = 0;

// static struct logger_t *_logger = &default_logger;

// static void logger_do_cat(struct logger_t *logger, struct logger_layout_t *layout, int level, const char *fmt, va_list ap)
// {
//     int n;
//     struct logger_appender_t *appender;

//     logger_lock(logger);

//     n = vsnprintf(logger_buffer(logger), logger_buffer_size(logger), fmt, ap);

//     for (appender = layout->appenders; appender ; appender = appender->next)
//     {
//         appender->writer(appender, level, logger_buffer(logger), n);
//     }

//     logger_unlock(logger);
// }

// static void logger_do_format_bin(struct logger_t *logger, struct logger_layout_t *layout, int level, const char *file, unsigned int line, const void *buf, unsigned int len)
// {
//     int n;
//     unsigned int ofs;
//     struct logger_appender_t *appender;
//     const unsigned char *p = (const unsigned char *)buf;

//     logger_lock(logger);

//     for (ofs = 0; ofs < len; ofs += 16)
//     {
//         n = layout->format_bin(layout, logger_buffer(logger), logger_buffer_size(logger), level,
//                 file, line, p + ofs, MIN(len - ofs, 16));

//         for (appender = layout->appenders; appender ; appender = appender->next)
//         {
//             appender->writer(appender, level, logger_buffer(logger), n);
//         }
//     }

//     logger_unlock(logger);
// }

// static void logger_do_format_str(struct logger_t *logger, struct logger_layout_t *layout, int level, const char *file, unsigned int line, const char *fmt, va_list ap)
// {
//     int n;
//     struct logger_appender_t *appender;

//     logger_lock(logger);

//     n = layout->format_str(layout, logger_buffer(logger), logger_buffer_size(logger), level, file, line, fmt, ap);

//     for (appender = layout->appenders; appender ; appender = appender->next)
//     {
//         appender->writer(appender, level, logger_buffer(logger), n);
//     }

//     logger_unlock(logger);
// }

// static void logger_vcatf(struct logger_t *logger, int level, const char *fmt, va_list ap)
// {
//     struct logger_layout_t *layout;

//     for (layout = logger->layouts; layout ; layout = layout->next)
//     {
//         logger_do_cat(logger, layout, level, fmt, ap);
//     }
// }

// void logger_vprintf(struct logger_t *logger, int level, const char *file, unsigned int line, const char *fmt, va_list ap)
// {
//     struct logger_layout_t *layout;

//     // global level filter
//     if (level != (level & logger->level_mask))
//     {
//         return;
//     }

//     for (layout = logger->layouts; layout ; layout = layout->next)
//     {
//         if (layout->format_str)
//         {
//             logger_do_format_str(logger, layout, level, file, line, fmt, ap);
//         }
//     }
// }

// void logger_catf(struct logger_t *logger, int level, const char *fmt, ...)
// {
//     va_list ap;
//     va_start(ap, fmt);
//     logger_vcatf(logger, level, fmt, ap);
//     va_end(ap);
// }

// void logger_printf(struct logger_t *logger, int level, const char *file, unsigned int line, const char *fmt, ...)
// {
//     va_list ap;
//     va_start(ap, fmt);
//     logger_vprintf(logger, level, file, line, fmt, ap);
//     va_end(ap);
// }

// void logger_printb(struct logger_t *logger, int level, const char *file, unsigned int line, const void *buf, int len)
// {
//     struct logger_layout_t *layout;

//     // global level filter
//     if (level != (level & logger->level_mask))
//     {
//         return;
//     }

//     for (layout = logger->layouts; layout ; layout = layout->next)
//     {
//         if (layout->format_bin)
//         {
//             logger_do_format_bin(logger, layout, level, file, line, buf, len);
//         }
//     }
// }

// void logger_layout_add_appender(struct logger_layout_t *layout, struct logger_appender_t *appender)
// {
//     appender->next = layout->appenders;
//     layout->appenders = appender;
// }

// void logger_add_layout(struct logger_t *logger, struct logger_layout_t *layout)
// {
//     struct logger_layout_t *cur, *prev;

//     logger_lock(logger);
//     if (logger->layouts) {
//         for (cur = logger->layouts; cur; prev = cur, cur = cur->next);
//         prev->next = layout;
//     } else {
//         logger->layouts = layout;
//     }
//     layout->next = NULL;
//     layout->logger = logger;
//     logger_unlock(logger);
// }

// void logger_set_level_mask(struct logger_t *logger, int level_mask)
// {
//     logger->level_mask = level_mask;
// }

// void logger_init(struct logger_t *logger, struct LOGGER_CFG *cfg)
// {
//     memset(logger, 0, sizeof(struct logger_t));
//     memcpy(&logger->cfg, cfg, sizeof(struct LOGGER_CFG));
//     logger->level_mask = LOG_LEVEL_ALL;
// }

void logger_add_appender(struct logger_t *logger, struct logger_appender_t *appender) {
    if (!logger || !appender) {
        return;
    }

    if (!logger->appenders) {
        logger->appenders = appender;
    } else {
        struct logger_appender_t *cur, *prev;
        for (cur = logger->appenders; cur; prev = cur, cur = cur->next);
        prev->next = appender;
    }
}

void logger_remove_appender(struct logger_t *logger, struct logger_appender_t *appender) {
    if (!logger || !appender) {
        return;
    }

    if (logger->appenders == appender) {
        logger->appenders = appender->next;
    } else {
        struct logger_appender_t *cur, *prev;
        for (cur = logger->appenders; cur; prev = cur, cur = cur->next) {
            if (cur == appender) {
                prev->next = cur->next;
            }
        }
    }
}

void logger_set_default_logger(struct logger_t *logger) {
    _default_logger = logger;
}

struct logger_t *logger_get_default_logger() {
    return _default_logger;
}

int logger_init(struct logger_t *logger, char *buf, int buf_size) {
    if (!logger) {
        return -1;
    }
    
    memset(logger, 0, sizeof(struct logger_t));
    
    if (buf && buf_size > 0) {
        logger->buf = buf;
        logger->buf_size = buf_size;
        logger->own_buf = 0;
    } else {
        logger->buf = logger_malloc(LOGGER_DEFAULT_LINE_SIZE);
        if (!logger->buf) {
            return -1;
        }
        logger->buf_size = LOGGER_DEFAULT_LINE_SIZE;
        logger->own_buf = 1;
    }
    
    logger_lock_new(&logger->lock);
    if (!logger->lock) {
        if (logger->own_buf) {
            logger_free(logger->buf);
        }
        logger->buf = NULL;
        logger->buf_size = 0;
        return -1;
    }
    
    return 0;
}

int logger_deinit(struct logger_t *logger) {
    if (!logger) {
        return -1;
    }
    
    if (logger->lock != NULL) {
        logger_lock_free(&logger->lock);
    }

    if (logger->buf != NULL && logger->own_buf) {
        logger_free(logger->buf);
    }
    
    logger->buf = NULL;
    logger->buf_size = 0;
    logger->own_buf = 0;
    
    return 0;
}

int logger_quick_startup(void) {
    if (_logger_initialized) {
        return 0; // Already initialized
    }
    
    // In multithreaded environments, we should lock to prevent double initialization
    // But since we don't have access to the logger lock here, we'll rely on the volatile flag
    // In a production environment, a separate initialization lock would be better
    
    int result = logger_init(&_logger, NULL, 0);
    if (result == 0) {
        _logger_initialized = 1;
    }
    return result;
}

int logger_quick_cleanup(void) {
    if (!_logger_initialized) {
        return 0; // Not initialized
    }
    
    int result = logger_deinit(&_logger);
    if (result == 0) {
        _logger_initialized = 0;
    }
    return result;
}
