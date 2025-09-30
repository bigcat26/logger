#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "logger/logger.h"
#include "logger/osal.h"

/* Forward declarations */
static int console_appender_write_func(logger_appender_t *appender, const struct logger_event_t *event);
static int file_appender_write_func(logger_appender_t *appender, const struct logger_event_t *event);
static int syslog_appender_write_func(logger_appender_t *appender, const struct logger_event_t *event);
static int udp_appender_write_func(logger_appender_t *appender, const struct logger_event_t *event);
static const char *get_level_color(logger_appender_t *appender, log_level_t level);
static int get_syslog_priority(log_level_t level);
static void check_and_rotate_file(logger_appender_file_config_t *cfg);
static void send_syslog_message(logger_appender_syslog_config_t *cfg, const char *message, int len);
static void send_udp_message(logger_appender_udp_config_t *cfg, const char *message, int len);

/* Appender Management API Implementation */

logger_appender_t *logger_appender_create_console(const logger_appender_console_config_t *config) {
    logger_appender_t *appender = logger_malloc(sizeof(logger_appender_t));
    if (!appender) {
        return NULL;
    }
    
    memset(appender, 0, sizeof(logger_appender_t));
    
    /* Initialize console appender */
    appender->write = console_appender_write_func;
    appender->buffer_size = LOGGER_DEFAULT_LINE_SIZE;
    appender->buffer = logger_malloc(appender->buffer_size);
    if (!appender->buffer) {
        logger_free(appender);
        return NULL;
    }
    
    /* Set configuration */
    if (config) {
        logger_appender_console_config_t *cfg = logger_malloc(sizeof(logger_appender_console_config_t));
        if (cfg) {
            memcpy(cfg, config, sizeof(logger_appender_console_config_t));
            appender->user_data = cfg;
        }
    }
    
    /* Initialize thread lock */
    logger_lock_new(&appender->lock);
    
    return appender;
}

logger_appender_t *logger_appender_create_file(const logger_appender_file_config_t *config) {
    if (!config || !config->filename) {
        return NULL;
    }
    
    logger_appender_t *appender = logger_malloc(sizeof(logger_appender_t));
    if (!appender) {
        return NULL;
    }
    
    memset(appender, 0, sizeof(logger_appender_t));
    
    /* Initialize file appender */
    appender->write = file_appender_write_func;
    appender->buffer_size = LOGGER_DEFAULT_LINE_SIZE;
    appender->buffer = logger_malloc(appender->buffer_size);
    if (!appender->buffer) {
        logger_free(appender);
        return NULL;
    }
    
    /* Set configuration */
    logger_appender_file_config_t *cfg = logger_malloc(sizeof(logger_appender_file_config_t));
    if (!cfg) {
        logger_free(appender->buffer);
        logger_free(appender);
        return NULL;
    }
    
    memcpy(cfg, config, sizeof(logger_appender_file_config_t));
    appender->user_data = cfg;
    
    /* Initialize thread lock */
    logger_lock_new(&appender->lock);
    
    return appender;
}

logger_appender_t *logger_appender_create_syslog(const logger_appender_syslog_config_t *config) {
    logger_appender_t *appender = logger_malloc(sizeof(logger_appender_t));
    if (!appender) {
        return NULL;
    }
    
    memset(appender, 0, sizeof(logger_appender_t));
    
    /* Initialize syslog appender */
    appender->write = syslog_appender_write_func;
    appender->buffer_size = LOGGER_DEFAULT_LINE_SIZE;
    appender->buffer = logger_malloc(appender->buffer_size);
    if (!appender->buffer) {
        logger_free(appender);
        return NULL;
    }
    
    /* Set configuration */
    if (config) {
        logger_appender_syslog_config_t *cfg = logger_malloc(sizeof(logger_appender_syslog_config_t));
        if (cfg) {
            memcpy(cfg, config, sizeof(logger_appender_syslog_config_t));
            appender->user_data = cfg;
        }
    }
    
    /* Initialize thread lock */
    logger_lock_new(&appender->lock);
    
    return appender;
}

logger_appender_t *logger_appender_create_udp(const logger_appender_udp_config_t *config) {
    if (!config || !config->host) {
        return NULL;
    }
    
    logger_appender_t *appender = logger_malloc(sizeof(logger_appender_t));
    if (!appender) {
        return NULL;
    }
    
    memset(appender, 0, sizeof(logger_appender_t));
    
    /* Initialize UDP appender */
    appender->write = udp_appender_write_func;
    appender->buffer_size = LOGGER_DEFAULT_LINE_SIZE;
    appender->buffer = logger_malloc(appender->buffer_size);
    if (!appender->buffer) {
        logger_free(appender);
        return NULL;
    }
    
    /* Set configuration */
    logger_appender_udp_config_t *cfg = logger_malloc(sizeof(logger_appender_udp_config_t));
    if (!cfg) {
        logger_free(appender->buffer);
        logger_free(appender);
        return NULL;
    }
    
    memcpy(cfg, config, sizeof(logger_appender_udp_config_t));
    appender->user_data = cfg;
    
    /* Initialize thread lock */
    logger_lock_new(&appender->lock);
    
    return appender;
}

int logger_appender_destroy(logger_appender_t *appender) {
    if (!appender) {
        return -1;
    }
    
    /* Free filters */
    logger_filter_t *filter = appender->filters;
    while (filter) {
        logger_filter_t *next = filter->next;
        logger_filter_destroy(filter);
        filter = next;
    }
    
    /* Don't free layout - it's managed by the caller */
    
    /* Free thread lock */
    if (appender->lock) {
        logger_lock_free(&appender->lock);
    }
    
    /* Free buffer */
    if (appender->buffer) {
        logger_free(appender->buffer);
    }
    
    /* Free user data */
    if (appender->user_data) {
        logger_free(appender->user_data);
    }
    
    /* Free appender itself */
    logger_free(appender);
    
    return 0;
}

/* Appender Write Functions */

int console_appender_write_func(logger_appender_t *appender, const struct logger_event_t *event) {
    if (!appender || !event) {
        return -1;
    }
    
    logger_lock_acquire(appender->lock);
    
    /* Format the message using layout */
    int len = 0;
    if (appender->layout && appender->layout->format) {
        len = appender->layout->format(appender->layout, event, appender->buffer, appender->buffer_size);
    } else {
        /* Default simple format */
        len = snprintf(appender->buffer, appender->buffer_size, "[%s] %s\n", 
                      logger_level_to_string(event->level), event->data);
    }
    
    if (len > 0 && len < (int)appender->buffer_size) {
        /* Apply colors if enabled */
        logger_appender_console_config_t *cfg = (logger_appender_console_config_t *)appender->user_data;
        if (cfg && cfg->enable_colors) {
            const char *color = get_level_color(appender, event->level);
            if (color) {
                printf("%s%s%s", color, appender->buffer, LOGGER_NONE);
            } else {
                printf("%s", appender->buffer);
            }
        } else {
            printf("%s", appender->buffer);
        }
        fflush(stdout);
    }
    
    logger_lock_release(appender->lock);
    return len;
}

int file_appender_write_func(logger_appender_t *appender, const struct logger_event_t *event) {
    if (!appender || !event) {
        return -1;
    }
    
    logger_lock_acquire(appender->lock);
    
    logger_appender_file_config_t *cfg = (logger_appender_file_config_t *)appender->user_data;
    if (!cfg) {
        logger_lock_release(appender->lock);
        return -1;
    }
    
    /* Format the message using layout */
    int len = 0;
    if (appender->layout && appender->layout->format) {
        len = appender->layout->format(appender->layout, event, appender->buffer, appender->buffer_size);
    } else {
        /* Default simple format */
        len = snprintf(appender->buffer, appender->buffer_size, "[%s] %s\n", 
                      logger_level_to_string(event->level), event->data);
    }
    
    if (len > 0 && len < (int)appender->buffer_size) {
        /* Write to file */
        FILE *fp = fopen(cfg->filename, "a");
        if (fp) {
            fwrite(appender->buffer, 1, len, fp);
            fclose(fp);
            
            /* Check file size and rotate if needed */
            if (cfg->max_file_size > 0) {
                check_and_rotate_file(cfg);
            }
        }
    }
    
    logger_lock_release(appender->lock);
    return len;
}

int syslog_appender_write_func(logger_appender_t *appender, const struct logger_event_t *event) {
    if (!appender || !event) {
        return -1;
    }
    
    logger_lock_acquire(appender->lock);
    
    /* Format the message using layout */
    int len = 0;
    if (appender->layout && appender->layout->format) {
        len = appender->layout->format(appender->layout, event, appender->buffer, appender->buffer_size);
    } else {
        /* Default syslog format */
        len = snprintf(appender->buffer, appender->buffer_size, "<%d>%s %s", 
                      get_syslog_priority(event->level), 
                      logger_level_to_string(event->level), event->data);
    }
    
    if (len > 0 && len < (int)appender->buffer_size) {
        /* Send to syslog server */
        logger_appender_syslog_config_t *cfg = (logger_appender_syslog_config_t *)appender->user_data;
        if (cfg) {
            send_syslog_message(cfg, appender->buffer, len);
        }
    }
    
    logger_lock_release(appender->lock);
    return len;
}

int udp_appender_write_func(logger_appender_t *appender, const struct logger_event_t *event) {
    if (!appender || !event) {
        return -1;
    }
    
    logger_lock_acquire(appender->lock);
    
    /* Format the message using layout */
    int len = 0;
    if (appender->layout && appender->layout->format) {
        len = appender->layout->format(appender->layout, event, appender->buffer, appender->buffer_size);
    } else {
        /* Default simple format */
        len = snprintf(appender->buffer, appender->buffer_size, "[%s] %s\n", 
                      logger_level_to_string(event->level), event->data);
    }
    
    if (len > 0 && len < (int)appender->buffer_size) {
        /* Send via UDP */
        logger_appender_udp_config_t *cfg = (logger_appender_udp_config_t *)appender->user_data;
        if (cfg) {
            send_udp_message(cfg, appender->buffer, len);
        }
    }
    
    logger_lock_release(appender->lock);
    return len;
}

/* Helper Functions */

static const char *get_level_color(logger_appender_t *appender, log_level_t level) {
    switch (level) {
        case LOG_LEVEL_VERBOSE: return LOGGER_GRAY;
        case LOG_LEVEL_DEBUG:   return LOGGER_CYAN;
        case LOG_LEVEL_INFO:    return LOGGER_GREEN;
        case LOG_LEVEL_WARN:    return LOGGER_YELLOW;
        case LOG_LEVEL_ERROR:   return LOGGER_RED;
        case LOG_LEVEL_FATAL:   return LOGGER_L_RED;
        default:                return NULL;
    }
}

static int get_syslog_priority(log_level_t level) {
    switch (level) {
        case LOG_LEVEL_VERBOSE: return 7; /* DEBUG */
        case LOG_LEVEL_DEBUG:   return 7; /* DEBUG */
        case LOG_LEVEL_INFO:    return 6; /* INFO */
        case LOG_LEVEL_WARN:    return 4; /* WARNING */
        case LOG_LEVEL_ERROR:   return 3; /* ERROR */
        case LOG_LEVEL_FATAL:   return 2; /* CRITICAL */
        default:                return 6; /* INFO */
    }
}

static void check_and_rotate_file(logger_appender_file_config_t *cfg) {
    /* This is a simplified implementation */
    /* In a real implementation, you would check file size and rotate files */
    /* For now, we'll just leave it as a placeholder */
}

static void send_syslog_message(logger_appender_syslog_config_t *cfg, const char *message, int len) {
    /* This is a placeholder for syslog message sending */
    /* In a real implementation, you would implement UDP socket communication */
    /* to send messages to the syslog server */
}

static void send_udp_message(logger_appender_udp_config_t *cfg, const char *message, int len) {
    /* This is a placeholder for UDP message sending */
    /* In a real implementation, you would implement UDP socket communication */
    /* to send messages to the UDP server */
}
