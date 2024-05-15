#include <ctype.h>

#include "logger/logger.h"
#include "logger/layout_simple.h"

#if defined(_WIN32)
// for vc2008
#if (_MSC_VER <= 1500)
#define snprintf    _snprintf
#endif
#endif

#if defined(_WIN32) || defined(linux) || defined(__APPLE__)
#include <stdio.h>

static int logger_format_str(struct LOGGER_LAYOUT *layout, char *buf, int size, int level, const char *file, unsigned int line, const char *fmt, va_list ap)
{
    return vsnprintf(buf, size, fmt, ap);
}

static int logger_format_bin(struct LOGGER_LAYOUT *layout, char *buf, int size, int level, const char *file, unsigned int line, const void *dat, int len)
{
    int n;
    int offset = 0;
    const unsigned char *p = (const unsigned char *)dat;

    // put address
    offset += snprintf(buf + offset, size - offset, "[%08X] ", (unsigned int)dat);

    // put hex
    for (n = 0; n < len; ++n)
    {
        offset += snprintf(buf + offset, size - offset, "%02X ", p[n]);
    }

    // paddings
    for (; n < 16; ++n)
    {
        offset += snprintf(buf + offset, size - offset, "   ");
    }

    // put char
    for (n = 0; n < len; ++n)
    {
        offset += snprintf(buf + offset, size - offset, "%c", isprint(p[n]) ? p[n] : '.');
    }

    offset += snprintf(buf + offset, size - offset, LOGGER_ENDL);
    return offset;
}

int logger_layout_simple_get_size()
{
    return sizeof(struct LOGGER_LAYOUT);
}

int logger_layout_simple_init(struct LOGGER_LAYOUT *layout)
{
    memset(layout, 0, sizeof(struct LOGGER_LAYOUT));
    layout->format_str = logger_format_str;
    layout->format_bin = logger_format_bin;
    return 0;
}

#endif
