#include <time.h>
#include <stdio.h>
#include <ctype.h>

#include "logger/logger.h"
#include "logger/layout_full.h"

#if defined(_WIN32)
#define snprintf _snprintf
#define LOGGER_PATH_SLASH       '\\'
#else
#define LOGGER_PATH_SLASH       '/'
#endif

static char *s_log_levels[] =
{
    "VERB ",
    "DEBUG",
    "INFO ",
    "WARN ",
    "ERROR",
    "FATAL"
};

static char logger_level_to_char(int level)
{
    int lv;
    static char logger_level_table[] = "VDIWEF";
    lv = level & LOGGER_LEVEL_MASK;
    lv = lv > LOGGER_LEVEL_FATAL ? LOGGER_LEVEL_FATAL : lv;
    lv = lv < LOGGER_LEVEL_VERBOSE ? LOGGER_LEVEL_VERBOSE : lv;
    return logger_level_table[lv - 1];
}

static int logger_format_str(struct LOGGER_LAYOUT *layout, char *buf, int size, int level, const char *file, unsigned int line, const char *fmt, va_list ap)
{
    int n;
    int lv;
    int ofs = 0;
    struct tm t;
    time_t now;
    const char *filename = NULL;

    layout->logger->cfg.syscall_time(&now);
    t = *layout->logger->cfg.syscall_localtime(&now);

    n = snprintf(buf + ofs, size - ofs, "%02d-%02d %02d:%02d:%02d|",
        t.tm_mon + 1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec);

    if (n > 0)
    {
        ofs += n;
    }

    // level
    lv = level & LOGGER_LEVEL_MASK;
    lv = lv > LOGGER_LEVEL_FATAL ? LOGGER_LEVEL_FATAL : lv;
    lv = lv < LOGGER_LEVEL_VERBOSE ? LOGGER_LEVEL_VERBOSE : lv;
    lv -= 1;
    n = snprintf(buf + ofs, size - ofs, "%s|", s_log_levels[lv]);

    if (n > 0)
    {
        ofs += n;
    }

    // position
    if (file != NULL && line > 0)
    {
        filename = strrchr(file, LOGGER_PATH_SLASH);

        if (filename == NULL)
        {
            filename = file;
        }
        else
        {
            filename = filename + 1;
        }

        n = snprintf(buf + ofs, size - ofs, "%-8.8s:%5d|", filename, line);

        if (n > 0)
        {
            ofs += n;
        }
    }

    ofs += vsnprintf(buf + ofs, size - ofs, fmt, ap);
    return ofs;
}

static int logger_format_bin(struct LOGGER_LAYOUT *layout, char *buf, int size, int level, const char *file, unsigned int line, const void *dat, int len)
{
    int n;
    int lv;
    int ofs = 0;
    time_t now;
    struct tm t;
    const char *filename = NULL;
    const unsigned char *p = (const unsigned char *)dat;

    layout->logger->cfg.syscall_time(&now);
    t = *layout->logger->cfg.syscall_localtime(&now);
    n = snprintf(buf + ofs, size - ofs, "%02d-%02d %02d:%02d:%02d|",
        t.tm_mon + 1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec);
    if (n > 0) {
        ofs += n;
    }

    // level
    lv = level & LOGGER_LEVEL_MASK;
    lv = lv > LOGGER_LEVEL_FATAL ? LOGGER_LEVEL_FATAL : lv;
    lv = lv < LOGGER_LEVEL_VERBOSE ? LOGGER_LEVEL_VERBOSE : lv;
    lv -= 1;
    n = snprintf(buf + ofs, size - ofs, "%s|", s_log_levels[lv]);
    if (n > 0) {
        ofs += n;
    }

    // position
    if (file != NULL && line > 0)
    {
        filename = strrchr(file, LOGGER_PATH_SLASH);

        if (filename == NULL)
        {
            filename = file;
        }
        else
        {
            filename = filename + 1;
        }

        n = snprintf(buf + ofs, size - ofs, "%-8.8s:%5d|", filename, line);
        if (n > 0)
        {
            ofs += n;
        }
    }

    // put address
    ofs += snprintf(buf + ofs, size - ofs, "[%p] ", dat);

    // put hex
    for (n = 0; n < len; ++n)
    {
        ofs += snprintf(buf + ofs, size - ofs, "%02X ", p[n]);
    }

    // paddings
    for (; n < 16; ++n)
    {
        ofs += snprintf(buf + ofs, size - ofs, "   ");
    }

    // put char
    for (n = 0; n < len; ++n)
    {
        ofs += snprintf(buf + ofs, size - ofs, "%c", isprint(p[n]) ? p[n] : '.');
    }

    ofs += snprintf(buf + ofs, size - ofs, LOGGER_ENDL);
    return ofs;
}

int logger_layout_full_get_size()
{
    return sizeof(struct LOGGER_LAYOUT);
}

int logger_layout_full_init(struct LOGGER_LAYOUT *layout)
{
    memset(layout, 0, sizeof(struct LOGGER_LAYOUT));
    layout->format_str = logger_format_str;
    layout->format_bin = logger_format_bin;
    return 0;
}
