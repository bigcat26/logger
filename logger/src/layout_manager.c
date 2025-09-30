#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "logger/logger.h"
#include "logger/osal.h"

/* Forward declarations */
static int full_layout_format_func(logger_layout_t *layout, const struct logger_event_t *event, 
                                  char *buffer, int buffer_size);
static int classic_layout_format_func(logger_layout_t *layout, const struct logger_event_t *event, 
                                    char *buffer, int buffer_size);
static int simple_layout_format_func(logger_layout_t *layout, const struct logger_event_t *event, 
                                    char *buffer, int buffer_size);
static int syslog_layout_format_func(logger_layout_t *layout, const struct logger_event_t *event, 
                                    char *buffer, int buffer_size);
static int get_syslog_priority(log_level_t level);
static void logger_get_hostname(char *hostname, size_t size);

/* Layout Management API Implementation */

logger_layout_t *logger_layout_create_full(void) {
    logger_layout_t *layout = logger_malloc(sizeof(logger_layout_t));
    if (!layout) {
        return NULL;
    }
    
    memset(layout, 0, sizeof(logger_layout_t));
    layout->format = full_layout_format_func;
    
    return layout;
}

logger_layout_t *logger_layout_create_classic(void) {
    logger_layout_t *layout = logger_malloc(sizeof(logger_layout_t));
    if (!layout) {
        return NULL;
    }
    
    memset(layout, 0, sizeof(logger_layout_t));
    layout->format = classic_layout_format_func;
    
    return layout;
}

logger_layout_t *logger_layout_create_simple(void) {
    logger_layout_t *layout = logger_malloc(sizeof(logger_layout_t));
    if (!layout) {
        return NULL;
    }
    
    memset(layout, 0, sizeof(logger_layout_t));
    layout->format = simple_layout_format_func;
    
    return layout;
}

logger_layout_t *logger_layout_create_syslog(void) {
    logger_layout_t *layout = logger_malloc(sizeof(logger_layout_t));
    if (!layout) {
        return NULL;
    }
    
    memset(layout, 0, sizeof(logger_layout_t));
    layout->format = syslog_layout_format_func;
    
    return layout;
}

int logger_appender_set_layout(logger_appender_t *appender, logger_layout_t *layout) {
    if (!appender) {
        return -1;
    }
    
    logger_lock_acquire(appender->lock);
    
    /* Don't free existing layout - let the caller handle it */
    appender->layout = layout;
    
    logger_lock_release(appender->lock);
    return 0;
}

int logger_layout_destroy(logger_layout_t *layout) {
    if (!layout) {
        return -1;
    }
    
    /* Free user data if any */
    if (layout->user_data) {
        logger_free(layout->user_data);
    }
    
    /* Free layout itself */
    logger_free(layout);
    
    return 0;
}

/* Layout Format Functions */

int full_layout_format_func(logger_layout_t *layout, const struct logger_event_t *event, 
                       char *buffer, int buffer_size) {
    if (!event || !buffer || buffer_size <= 0) {
        return -1;
    }
    
    /* Format: [YYYY-MM-DD HH:MM:SS.mmm] [LEVEL] [TID] [FILE:LINE:FUNC] MESSAGE */
    struct tm *tm_info;
    time_t timestamp_sec = event->timestamp_ns / 1000000000;
    uint32_t timestamp_ms = (event->timestamp_ns % 1000000000) / 1000000;
    
    tm_info = localtime(&timestamp_sec);
    if (!tm_info) {
        return -1;
    }
    
    const char *level_str = logger_level_to_string(event->level);
    const char *file = event->file ? event->file : "unknown";
    const char *func = event->func ? event->func : "unknown";
    
    int len = snprintf(buffer, buffer_size,
                      "%04d-%02d-%02d %02d:%02d:%02d.%03d %s [T%u] [%s:%u:%s] %s\n",
                      tm_info->tm_year + 1900, tm_info->tm_mon + 1, tm_info->tm_mday,
                      tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec, timestamp_ms,
                      level_str, event->tid, file, event->line, func, event->data);
    
    return (len < buffer_size) ? len : buffer_size - 1;
}

int classic_layout_format_func(logger_layout_t *layout, const struct logger_event_t *event, 
                       char *buffer, int buffer_size) {
    if (!event || !buffer || buffer_size <= 0) {
        return -1;
    }
    
    /* Format: [YYYY-MM-DD HH:MM:SS.mmm] [LEVEL] [TID] [FILE:LINE:FUNC] MESSAGE */
    struct tm *tm_info;
    time_t timestamp_sec = event->timestamp_ns / 1000000000;
    uint32_t timestamp_ms = (event->timestamp_ns % 1000000000) / 1000000;
    
    tm_info = localtime(&timestamp_sec);
    if (!tm_info) {
        return -1;
    }
    
    const char *level_str = logger_level_to_string(event->level);
    
    int len = snprintf(buffer, buffer_size,
                      "%04d-%02d-%02d %02d:%02d:%02d.%03d %C %s\n",
                      tm_info->tm_year + 1900, tm_info->tm_mon + 1, tm_info->tm_mday,
                      tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec, timestamp_ms,
                      level_str[0], event->data);
    
    return (len < buffer_size) ? len : buffer_size - 1;
}

int simple_layout_format_func(logger_layout_t *layout, const struct logger_event_t *event, 
                        char *buffer, int buffer_size) {
    if (!event || !buffer || buffer_size <= 0) {
        return -1;
    }
    
    /* Format: [LEVEL] MESSAGE */
    const char *level_str = logger_level_to_string(event->level);
    
    int len = snprintf(buffer, buffer_size, "[%s] %s\n", level_str, event->data);
    
    return (len < buffer_size) ? len : buffer_size - 1;
}

int syslog_layout_format_func(logger_layout_t *layout, const struct logger_event_t *event, 
                        char *buffer, int buffer_size) {
    if (!event || !buffer || buffer_size <= 0) {
        return -1;
    }
    
    /* Format: <PRI>TIMESTAMP HOSTNAME TAG: MESSAGE */
    struct tm *tm_info;
    time_t timestamp_sec = event->timestamp_ns / 1000000000;
    
    tm_info = localtime(&timestamp_sec);
    if (!tm_info) {
        return -1;
    }
    
    /* Calculate syslog priority */
    int priority = get_syslog_priority(event->level);
    
    /* Format timestamp in RFC 3164 format */
    char timestamp_str[32];
    strftime(timestamp_str, sizeof(timestamp_str), "%b %d %H:%M:%S", tm_info);
    
    /* Get hostname */
    char hostname[256] = "localhost";
    logger_get_hostname(hostname, sizeof(hostname));
    
    /* Get tag (use file name without path) */
    char tag[64] = "logger";
    if (event->file) {
        const char *filename = strrchr(event->file, '/');
        if (filename) {
            filename++; /* Skip the '/' */
        } else {
            filename = event->file;
        }
        
        /* Remove extension */
        char *ext = strrchr(filename, '.');
        if (ext) {
            size_t tag_len = ext - filename;
            if (tag_len < sizeof(tag)) {
                strncpy(tag, filename, tag_len);
                tag[tag_len] = '\0';
            } else {
                strncpy(tag, filename, sizeof(tag) - 1);
                tag[sizeof(tag) - 1] = '\0';
            }
        } else {
            strncpy(tag, filename, sizeof(tag) - 1);
            tag[sizeof(tag) - 1] = '\0';
        }
    }
    
    int len = snprintf(buffer, buffer_size, "<%d>%s %s %s: %s\n",
                      priority, timestamp_str, hostname, tag, event->data);
    
    return (len < buffer_size) ? len : buffer_size - 1;
}

/* Helper Functions */

static int get_syslog_priority(log_level_t level) {
    switch (level) {
        case LOG_VERBOSE: return 7; /* DEBUG */
        case LOG_DEBUG:   return 7; /* DEBUG */
        case LOG_INFO:    return 6; /* INFO */
        case LOG_WARN:    return 4; /* WARNING */
        case LOG_ERROR:   return 3; /* ERROR */
        case LOG_FATAL:   return 2; /* CRITICAL */
        default:                return 6; /* INFO */
    }
}

static void logger_get_hostname(char *hostname, size_t size) {
    if (!hostname || size == 0) {
        return;
    }
    
    /* Simple implementation - just use localhost */
    strncpy(hostname, "localhost", size - 1);
    hostname[size - 1] = '\0';
}
