#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/utsname.h>
#include <stdbool.h>
#include "logger/logger.h"
#include "logger/layout_syslog.h"
#include "logger/osal.h"

/* Syslog layout internal structure */
typedef struct {
    int facility;
    char tag[32];
    int pid;
    char hostname[64];
    bool include_timestamp;
    bool include_hostname;
    bool include_tag;
    bool include_pid;
} syslog_layout_data_t;

/* Forward declarations */
static int syslog_layout_format_func(logger_layout_t *layout, const struct logger_event_t *event, 
                                    char *buffer, int buffer_size);
static int get_syslog_priority(log_level_t level);
static int get_syslog_facility(int facility);
static void logger_get_hostname(char *hostname, size_t size);
static int format_syslog_timestamp(char *buffer, size_t size, uint64_t timestamp_ns);
static int format_syslog_message(char *buffer, size_t size, const struct logger_event_t *event,
                                syslog_layout_data_t *data);

/* Syslog layout format function */
static int syslog_layout_format_func(logger_layout_t *layout, const struct logger_event_t *event, 
                                   char *buffer, int buffer_size) {
    if (!layout || !event || !buffer || buffer_size <= 0) {
        return -1;
    }
    
    syslog_layout_data_t *data = (syslog_layout_data_t *)layout->user_data;
    if (!data) {
        return -1;
    }
    
    return format_syslog_message(buffer, buffer_size, event, data);
}

/* Get syslog priority from log level */
static int get_syslog_priority(log_level_t level) {
    switch (level) {
        case LOG_FATAL:
            return SYSLOG_PRIORITY_CRIT;
        case LOG_ERROR:
            return SYSLOG_PRIORITY_ERR;
        case LOG_WARN:
            return SYSLOG_PRIORITY_WARNING;
        case LOG_INFO:
            return SYSLOG_PRIORITY_INFO;
        case LOG_DEBUG:
            return SYSLOG_PRIORITY_DEBUG;
        case LOG_VERBOSE:
            return SYSLOG_PRIORITY_DEBUG;
        default:
            return SYSLOG_PRIORITY_INFO;
    }
}

/* Get syslog facility */
static int get_syslog_facility(int facility) {
    switch (facility) {
        case SYSLOG_FACILITY_KERN:
        case SYSLOG_FACILITY_USER:
        case SYSLOG_FACILITY_MAIL:
        case SYSLOG_FACILITY_DAEMON:
        case SYSLOG_FACILITY_AUTH:
        case SYSLOG_FACILITY_SYSLOGD:
        case SYSLOG_FACILITY_LPR:
        case SYSLOG_FACILITY_NEWS:
        case SYSLOG_FACILITY_UUCP:
        case SYSLOG_FACILITY_CLOCK:
        case SYSLOG_FACILITY_AUTHPRI:
        case SYSLOG_FACILITY_FTP:
        case SYSLOG_FACILITY_NTP:
        case SYSLOG_FACILITY_LOG_AUDIT:
        case SYSLOG_FACILITY_LOG_ALERT:
        case SYSLOG_FACILITY_CRON:
        case SYSLOG_FACILITY_LOCAL0:
        case SYSLOG_FACILITY_LOCAL1:
        case SYSLOG_FACILITY_LOCAL2:
        case SYSLOG_FACILITY_LOCAL3:
        case SYSLOG_FACILITY_LOCAL4:
        case SYSLOG_FACILITY_LOCAL5:
        case SYSLOG_FACILITY_LOCAL6:
        case SYSLOG_FACILITY_LOCAL7:
            return facility;
        default:
            return SYSLOG_FACILITY_USER;
    }
}

/* Get hostname */
static void logger_get_hostname(char *hostname, size_t size) {
    if (!hostname || size == 0) {
        return;
    }
    
    if (gethostname(hostname, size - 1) != 0) {
        strncpy(hostname, "unknown", size - 1);
    }
    hostname[size - 1] = '\0';
}

/* Format syslog timestamp */
static int format_syslog_timestamp(char *buffer, size_t size, uint64_t timestamp_ns) {
    if (!buffer || size == 0) {
        return -1;
    }
    
    time_t timestamp = timestamp_ns / 1000000000; /* Convert nanoseconds to seconds */
    struct tm *tm_info = localtime(&timestamp);
    
    if (!tm_info) {
        return -1;
    }
    
    /* Format: MMM DD HH:MM:SS (RFC 3164) */
    const char *months[] = {
        "Jan", "Feb", "Mar", "Apr", "May", "Jun",
        "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
    };
    
    return snprintf(buffer, size, "%s %2d %02d:%02d:%02d",
                   months[tm_info->tm_mon],
                   tm_info->tm_mday,
                   tm_info->tm_hour,
                   tm_info->tm_min,
                   tm_info->tm_sec);
}

/* Format complete syslog message */
static int format_syslog_message(char *buffer, size_t size, const struct logger_event_t *event,
                                syslog_layout_data_t *data) {
    if (!buffer || size == 0 || !event || !data) {
        return -1;
    }
    
    int priority = get_syslog_priority(event->level);
    int facility = get_syslog_facility(data->facility);
    int pri = facility * 8 + priority;
    
    int offset = 0;
    int remaining = size;
    
    /* Format priority */
    int written = snprintf(buffer + offset, remaining, "<%d>", pri);
    if (written < 0 || written >= remaining) {
        return -1;
    }
    offset += written;
    remaining -= written;
    
    /* Format timestamp */
    if (data->include_timestamp) {
        char timestamp[32];
        if (format_syslog_timestamp(timestamp, sizeof(timestamp), event->timestamp_ns) > 0) {
            written = snprintf(buffer + offset, remaining, "%s ", timestamp);
            if (written < 0 || written >= remaining) {
                return -1;
            }
            offset += written;
            remaining -= written;
        }
    }
    
    /* Format hostname */
    if (data->include_hostname) {
        written = snprintf(buffer + offset, remaining, "%s ", data->hostname);
        if (written < 0 || written >= remaining) {
            return -1;
        }
        offset += written;
        remaining -= written;
    }
    
    /* Format tag */
    if (data->include_tag && strlen(data->tag) > 0) {
        written = snprintf(buffer + offset, remaining, "%s", data->tag);
        if (written < 0 || written >= remaining) {
            return -1;
        }
        offset += written;
        remaining -= written;
        
        /* Add PID if enabled */
        if (data->include_pid) {
            written = snprintf(buffer + offset, remaining, "[%d]", data->pid);
            if (written < 0 || written >= remaining) {
                return -1;
            }
            offset += written;
            remaining -= written;
        }
        
        /* Add colon and space */
        written = snprintf(buffer + offset, remaining, ": ");
        if (written < 0 || written >= remaining) {
            return -1;
        }
        offset += written;
        remaining -= written;
    }
    
    /* Append the actual log message */
    if (remaining > 0 && event->data && event->msg_len > 0) {
        size_t msg_len = event->msg_len;
        if (msg_len >= remaining) {
            msg_len = remaining - 1;
        }
        
        memcpy(buffer + offset, event->data, msg_len);
        offset += msg_len;
        buffer[offset] = '\0';
    }
    
    return offset;
}

/* Public API Implementation */

int logger_layout_syslog_get_size(void) {
    return sizeof(struct logger_layout_t);
}

int logger_layout_syslog_init(struct logger_layout_t *layout, int facility, const char *tag, int pid) {
    if (!layout) {
        return -1;
    }
    
    /* Allocate internal data */
    syslog_layout_data_t *data = logger_malloc(sizeof(syslog_layout_data_t));
    if (!data) {
        return -1;
    }
    
    memset(data, 0, sizeof(syslog_layout_data_t));
    
    /* Set configuration */
    data->facility = facility;
    data->pid = pid;
    data->include_timestamp = true;
    data->include_hostname = true;
    data->include_tag = true;
    data->include_pid = true;
    
    /* Copy tag */
    if (tag) {
        strncpy(data->tag, tag, sizeof(data->tag) - 1);
        data->tag[sizeof(data->tag) - 1] = '\0';
    } else {
        strncpy(data->tag, "logger", sizeof(data->tag) - 1);
    }
    
    /* Get hostname */
    logger_get_hostname(data->hostname, sizeof(data->hostname));
    
    /* Initialize layout */
    layout->format = syslog_layout_format_func;
    layout->user_data = data;
    
    return 0;
}

/* Additional helper functions for syslog layout configuration */

int logger_layout_syslog_set_facility(struct logger_layout_t *layout, int facility) {
    if (!layout || !layout->user_data) {
        return -1;
    }
    
    syslog_layout_data_t *data = (syslog_layout_data_t *)layout->user_data;
    data->facility = facility;
    
    return 0;
}

int logger_layout_syslog_set_tag(struct logger_layout_t *layout, const char *tag) {
    if (!layout || !layout->user_data || !tag) {
        return -1;
    }
    
    syslog_layout_data_t *data = (syslog_layout_data_t *)layout->user_data;
    strncpy(data->tag, tag, sizeof(data->tag) - 1);
    data->tag[sizeof(data->tag) - 1] = '\0';
    
    return 0;
}

int logger_layout_syslog_set_pid(struct logger_layout_t *layout, int pid) {
    if (!layout || !layout->user_data) {
        return -1;
    }
    
    syslog_layout_data_t *data = (syslog_layout_data_t *)layout->user_data;
    data->pid = pid;
    
    return 0;
}

int logger_layout_syslog_set_options(struct logger_layout_t *layout, bool include_timestamp,
                                   bool include_hostname, bool include_tag, bool include_pid) {
    if (!layout || !layout->user_data) {
        return -1;
    }
    
    syslog_layout_data_t *data = (syslog_layout_data_t *)layout->user_data;
    data->include_timestamp = include_timestamp;
    data->include_hostname = include_hostname;
    data->include_tag = include_tag;
    data->include_pid = include_pid;
    
    return 0;
}

int logger_layout_syslog_deinit(struct logger_layout_t *layout) {
    if (!layout || !layout->user_data) {
        return -1;
    }
    
    syslog_layout_data_t *data = (syslog_layout_data_t *)layout->user_data;
    logger_free(data);
    layout->user_data = NULL;
    
    return 0;
}

/* Utility functions for syslog message creation */

int logger_create_syslog_message(char *buffer, size_t size, log_level_t level, int facility,
                                const char *tag, int pid, const char *message) {
    if (!buffer || size == 0 || !message) {
        return -1;
    }
    
    /* Create a temporary event structure */
    struct logger_event_t event = {0};
    event.level = level;
    event.timestamp_ns = logger_get_timestamp_ns();
    event.data = message;
    event.msg_len = strlen(message);
    
    /* Create temporary layout */
    logger_layout_t layout;
    if (logger_layout_syslog_init(&layout, facility, tag, pid) != 0) {
        return -1;
    }
    
    /* Format the message */
    int result = layout.format(&layout, &event, buffer, size);
    
    /* Cleanup */
    logger_layout_syslog_deinit(&layout);
    
    return result;
}

/* Test function for syslog layout */
int logger_layout_syslog_test(void) {
    char buffer[1024];
    struct logger_event_t event = {0};
    
    /* Setup test event */
    event.level = LOG_INFO;
    event.timestamp_ns = logger_get_timestamp_ns();
    event.data = "Test syslog message";
    event.msg_len = strlen(event.data);
    
    /* Create syslog layout */
    logger_layout_t layout;
    if (logger_layout_syslog_init(&layout, SYSLOG_FACILITY_USER, "test", getpid()) != 0) {
        return -1;
    }
    
    /* Format message */
    int result = layout.format(&layout, &event, buffer, sizeof(buffer));
    
    /* Cleanup */
    logger_layout_syslog_deinit(&layout);
    
    if (result > 0) {
        printf("Syslog test message: %s\n", buffer);
        return 0;
    }
    
    return -1;
}
