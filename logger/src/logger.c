#include <stdio.h>
#include "logger/logger.h"

#if defined(WIN32) && !defined(vsnprintf)
#define vsnprintf   _vsnprintf
#endif

#ifndef MIN
#define MIN(a, b)   ((a) > (b) ? (b) : (a))
#endif

#define logger_lock(logger)          do { if ((logger)->cfg.acquire_lock) { (logger)->cfg.acquire_lock((logger)->cfg.lock); } } while(0)
#define logger_unlock(logger)        do { if ((logger)->cfg.release_lock) { (logger)->cfg.release_lock((logger)->cfg.lock); } } while(0)

#define logger_buffer(logger)        ((logger)->cfg.buf)
#define logger_buffer_size(logger)   ((logger)->cfg.buf_size)

extern struct LOGGER default_logger;
static struct LOGGER *_logger = &default_logger;

static void logger_do_cat(struct LOGGER *logger, struct LOGGER_LAYOUT *layout, int level, const char *fmt, va_list ap)
{
    int n;
    struct LOGGER_APPENDER *appender;

    logger_lock(logger);

    n = vsnprintf(logger_buffer(logger), logger_buffer_size(logger), fmt, ap);

    for (appender = layout->appenders; appender ; appender = appender->next)
    {
        appender->writer(appender, level, logger_buffer(logger), n);
    }

    logger_unlock(logger);
}

static void logger_do_format_bin(struct LOGGER *logger, struct LOGGER_LAYOUT *layout, int level, const char *file, unsigned int line, const void *buf, unsigned int len)
{
    int n;
    unsigned int ofs;
    struct LOGGER_APPENDER *appender;
    const unsigned char *p = (const unsigned char *)buf;

    logger_lock(logger);

    for (ofs = 0; ofs < len; ofs += 16)
    {
        n = layout->format_bin(layout, logger_buffer(logger), logger_buffer_size(logger), level,
                file, line, p + ofs, MIN(len - ofs, 16));

        for (appender = layout->appenders; appender ; appender = appender->next)
        {
            appender->writer(appender, level, logger_buffer(logger), n);
        }
    }

    logger_unlock(logger);
}

static void logger_do_format_str(struct LOGGER *logger, struct LOGGER_LAYOUT *layout, int level, const char *file, unsigned int line, const char *fmt, va_list ap)
{
    int n;
    struct LOGGER_APPENDER *appender;

    logger_lock(logger);

    n = layout->format_str(layout, logger_buffer(logger), logger_buffer_size(logger), level, file, line, fmt, ap);

    for (appender = layout->appenders; appender ; appender = appender->next)
    {
        appender->writer(appender, level, logger_buffer(logger), n);
    }

    logger_unlock(logger);
}

static void logger_vcatf(struct LOGGER *logger, int level, const char *fmt, va_list ap)
{
    struct LOGGER_LAYOUT *layout;

    for (layout = logger->layouts; layout ; layout = layout->next)
    {
        logger_do_cat(logger, layout, level, fmt, ap);
    }
}

void logger_vprintf(struct LOGGER *logger, int level, const char *file, unsigned int line, const char *fmt, va_list ap)
{
    struct LOGGER_LAYOUT *layout;

    // global level filter
    if (level != (level & logger->level_mask))
    {
        return;
    }

    for (layout = logger->layouts; layout ; layout = layout->next)
    {
        if (layout->format_str)
        {
            logger_do_format_str(logger, layout, level, file, line, fmt, ap);
        }
    }
}

void logger_catf(struct LOGGER *logger, int level, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    logger_vcatf(logger, level, fmt, ap);
    va_end(ap);
}

void logger_printf(struct LOGGER *logger, int level, const char *file, unsigned int line, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    logger_vprintf(logger, level, file, line, fmt, ap);
    va_end(ap);
}

void logger_printb(struct LOGGER *logger, int level, const char *file, unsigned int line, const void *buf, int len)
{
    struct LOGGER_LAYOUT *layout;

    // global level filter
    if (level != (level & logger->level_mask))
    {
        return;
    }

    for (layout = logger->layouts; layout ; layout = layout->next)
    {
        if (layout->format_bin)
        {
            logger_do_format_bin(logger, layout, level, file, line, buf, len);
        }
    }
}

void logger_layout_add_appender(struct LOGGER_LAYOUT *layout, struct LOGGER_APPENDER *appender)
{
    appender->next = layout->appenders;
    layout->appenders = appender;
}

void logger_add_layout(struct LOGGER *logger, struct LOGGER_LAYOUT *layout)
{
    struct LOGGER_LAYOUT *cur, *prev;

    logger_lock(logger);
    if (logger->layouts) {
        for (cur = logger->layouts; cur; prev = cur, cur = cur->next);
        prev->next = layout;
    } else {
        logger->layouts = layout;
    }
    layout->next = NULL;
    layout->logger = logger;
    logger_unlock(logger);
}

void logger_set_level_mask(struct LOGGER *logger, int level_mask)
{
    logger->level_mask = level_mask;
}

void logger_init(struct LOGGER *logger, struct LOGGER_CFG *cfg)
{
    memset(logger, 0, sizeof(struct LOGGER));
    memcpy(&logger->cfg, cfg, sizeof(struct LOGGER_CFG));
    logger->level_mask = LOGGER_LEVEL_ALL;
}

void logger_set_default(struct LOGGER *logger)
{
    _logger = logger;
}

struct LOGGER *logger_get_default()
{
    return _logger;
}
