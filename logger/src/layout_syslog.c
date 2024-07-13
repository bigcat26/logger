#include <stdio.h>
#include <ctype.h>

#include "logger/logger.h"
#include "logger/layout_syslog.h"

#if defined(_WIN32)
// for vc2008
#if (_MSC_VER <= 1500)
#define snprintf    _snprintf
#endif
#endif

// TODO: implement RFC5424

struct SYSLOG_LOGGER_LAYOUT
{
    struct LOGGER_LAYOUT layout;
    int facility;
    int pid;
    char tag[33];
};

static int logger_level_to_syslog_severity(int level)
{
    switch (level) 
    {
    case LOGGER_LEVEL_VERBOSE:
    case LOGGER_LEVEL_DEBUG:
        return SYSLOG_PRIORITY_DEBUG;
    case LOGGER_LEVEL_INFO:
        return SYSLOG_PRIORITY_INFO;
    case LOGGER_LEVEL_WARN:
        return SYSLOG_PRIORITY_WARNING;
    case LOGGER_LEVEL_ERROR:
        return SYSLOG_PRIORITY_ERR;
    case LOGGER_LEVEL_FATAL:
        return SYSLOG_PRIORITY_EMERG;
    }
    return SYSLOG_PRIORITY_DEBUG;
}

static int logger_format_rfc3164_pri(char *buf, int size, int facility, int severity)
{
    int n = snprintf(buf, size, "<%d>", (facility << 3) + severity);
    if (n < 0 || n == size) {
        /* string too long */
        return -1;
    }
    return n;
}

static int logger_format_rfc3164_timestamp(char *buf, int size, struct tm *t)
{
    int n = strftime(buf, size, "%b %d %Y %X ", t);
    if (n < 0 || n == size) {
        /* string too long */
        return -1;
    }
    return n;
}

static int logger_format_rfc3164_tagpid(char *buf, int size, const char *tag, int pid)
{
    int n = 0;
    if (NULL != tag) {
        if (pid <= 0) {
            n = snprintf(buf, size, "%s: ", tag);
        } else {
            n = snprintf(buf, size, "%s[%d]: ", tag, pid);
        }
        if (n < 0 || n == size) {
            return -1;
        }
    }
    return n;
}

static int logger_format_remove_crlf_tail(char *buf, int size)
{
    // remove \r\n tail
    while (buf[size - 1] == '\r' || buf[size - 1] == '\n') {
        buf[size - 1] = 0;
        --size;
    }
    return size;
}

static int logger_format_str(struct LOGGER_LAYOUT *layout, char *buf, int size, int level, const char *file, unsigned int line, const char *fmt, va_list ap)
{
    /* syslog RFC3164 format: <30>Oct 9 22:33:20 hlfedora auditd[1787]: The audit daemon is exiting. */
    int n;
    int ofs = 0;
    time_t now;
    struct tm localtm;
    struct SYSLOG_LOGGER_LAYOUT *slayout = (struct SYSLOG_LOGGER_LAYOUT *)layout;

    layout->logger->cfg.syscall_time(&now);
    localtm = *layout->logger->cfg.syscall_localtime(&now);

    /* append facility & priority tag */
    n = logger_format_rfc3164_pri(buf + ofs, size - ofs, slayout->facility, logger_level_to_syslog_severity(level));
    if (n < 0) {
        /* string too long */
        goto __exit;
    }
    ofs += n;

    /* append time string */
    n = logger_format_rfc3164_timestamp(buf + ofs, size - ofs, &localtm);
    if (n < 0) {
        /* string too long */
        goto __exit;
    }
    ofs += n;

    n = logger_format_rfc3164_tagpid(buf + ofs, size - ofs, slayout->tag, slayout->pid);
    if (n < 0) {
        /* string too long */
        goto __exit;
    }
    ofs += n;

    /* append log content */
    n = vsnprintf(buf + ofs, size - ofs, fmt, ap);
    if (n < 0 || n == size - ofs) {
        /* string too long */
        goto __exit;
    }
    ofs += n;

__exit:
    ofs = logger_format_remove_crlf_tail(buf, ofs);
    return ofs;
}

static int logger_format_bin(struct LOGGER_LAYOUT *layout, char *buf, int size, int level, const char *file, unsigned int line, const void *dat, int len)
{
    int n;
    int ofs = 0;
    const unsigned char *p = (const unsigned char *)dat;
    time_t now;
    struct tm localtm;
    struct SYSLOG_LOGGER_LAYOUT *slayout = (struct SYSLOG_LOGGER_LAYOUT *)layout;

    layout->logger->cfg.syscall_time(&now);
    localtm = *layout->logger->cfg.syscall_localtime(&now);

    /* append facility & priority tag */
    n = logger_format_rfc3164_pri(buf + ofs, size - ofs, slayout->facility, logger_level_to_syslog_severity(level));
    if (n < 0) {
        /* string too long */
        goto __exit;
    }
    ofs += n;

    /* append time string */
    n = logger_format_rfc3164_timestamp(buf + ofs, size - ofs, &localtm);
    if (n < 0) {
        /* string too long */
        goto __exit;
    }
    ofs += n;

    n = logger_format_rfc3164_tagpid(buf + ofs, size - ofs, slayout->tag, slayout->pid);
    if (n < 0) {
        /* string too long */
        goto __exit;
    }
    ofs += n;

    // put address
    ofs += snprintf(buf + ofs, size - ofs, "[%p] ", dat);
    // put hex
    for (n = 0; n < len; ++n) {
        ofs += snprintf(buf + ofs, size - ofs, "%02X ", p[n]);
    }
    // paddings
    for (; n < 16; ++n) {
        ofs += snprintf(buf + ofs, size - ofs, "   ");
    }
    // put char
    for (n = 0; n < len; ++n) {
        ofs += snprintf(buf + ofs, size - ofs, "%c", isprint(p[n]) ? p[n] : '.');
    }
    ofs += snprintf(buf + ofs, size - ofs, LOGGER_ENDL);

__exit:
    ofs = logger_format_remove_crlf_tail(buf, ofs);
    return ofs;
}

int logger_layout_syslog_get_size()
{
    return sizeof(struct SYSLOG_LOGGER_LAYOUT);
}

int logger_layout_syslog_init(struct LOGGER_LAYOUT *layout, int facility, const char *tag, int pid)
{
    struct SYSLOG_LOGGER_LAYOUT *slayout = (struct SYSLOG_LOGGER_LAYOUT *)layout;
    memset(slayout, 0, sizeof(struct SYSLOG_LOGGER_LAYOUT));
    slayout->layout.format_str = logger_format_str;
    slayout->layout.format_bin = logger_format_bin;
    slayout->facility = facility;
    slayout->pid = pid;
    strncpy(slayout->tag, tag, sizeof(slayout->tag) - 1);
    return 0;
}
