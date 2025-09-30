#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "logger/logger.h"
#include "logger/osal.h"

/* Forward declarations */
static int level_filter_accept_func(logger_filter_t *filter, struct logger_event_t *event, void *user_data);
static int wildcard_match(const char *str, const char *pattern);

/* Filter Management API Implementation */

logger_filter_t *logger_filter_create_level(log_level_t min_level) {
    logger_filter_t *filter = logger_malloc(sizeof(logger_filter_t));
    if (!filter) {
        return NULL;
    }
    
    memset(filter, 0, sizeof(logger_filter_t));
    filter->accept = level_filter_accept_func;
    
    /* Store minimum level in user data */
    log_level_t *level_data = logger_malloc(sizeof(log_level_t));
    if (!level_data) {
        logger_free(filter);
        return NULL;
    }
    
    *level_data = min_level;
    filter->user_data = level_data;
    
    return filter;
}

logger_filter_t *logger_filter_create_custom(logger_filter_fn filter_fn, void *user_data) {
    if (!filter_fn) {
        return NULL;
    }
    
    logger_filter_t *filter = logger_malloc(sizeof(logger_filter_t));
    if (!filter) {
        return NULL;
    }
    
    memset(filter, 0, sizeof(logger_filter_t));
    filter->accept = filter_fn;
    filter->user_data = user_data;
    
    return filter;
}

int logger_appender_add_filter(logger_appender_t *appender, logger_filter_t *filter) {
    if (!appender || !filter) {
        return -1;
    }
    
    logger_lock_acquire(appender->lock);
    
    if (!appender->filters) {
        appender->filters = filter;
        filter->next = NULL;
    } else {
        logger_filter_t *cur = appender->filters;
        while (cur->next) {
            cur = cur->next;
        }
        cur->next = filter;
        filter->next = NULL;
    }
    
    logger_lock_release(appender->lock);
    return 0;
}

int logger_appender_remove_filter(logger_appender_t *appender, logger_filter_t *filter) {
    if (!appender || !filter) {
        return -1;
    }
    
    logger_lock_acquire(appender->lock);
    
    if (appender->filters == filter) {
        appender->filters = filter->next;
    } else {
        logger_filter_t *cur = appender->filters;
        while (cur && cur->next != filter) {
            cur = cur->next;
        }
        if (cur) {
            cur->next = filter->next;
        }
    }
    
    logger_lock_release(appender->lock);
    return 0;
}

int logger_filter_destroy(logger_filter_t *filter) {
    if (!filter) {
        return -1;
    }
    
    /* Free user data if it was allocated by us */
    if (filter->user_data && filter->accept == level_filter_accept_func) {
        logger_free(filter->user_data);
    }
    
    /* Free filter itself */
    logger_free(filter);
    
    return 0;
}

/* Filter Functions */

int level_filter_accept_func(logger_filter_t *filter, struct logger_event_t *event, void *user_data) {
    if (!filter || !event) {
        return 0;
    }
    
    log_level_t *min_level = (log_level_t *)filter->user_data;
    if (!min_level) {
        return 0;
    }
    
    /* Accept if event level is greater than or equal to minimum level */
    return (event->level >= *min_level) ? 1 : 0;
}

/* Common Filter Functions */

int keyword_filter_accept(logger_filter_t *filter, struct logger_event_t *event, void *user_data) {
    if (!filter || !event || !event->data) {
        return 0;
    }
    
    const char *keyword = (const char *)filter->user_data;
    if (!keyword) {
        return 0;
    }
    
    /* Accept if message contains the keyword */
    return (strstr(event->data, keyword) != NULL) ? 1 : 0;
}

int regex_filter_accept(logger_filter_t *filter, struct logger_event_t *event, void *user_data) {
    if (!filter || !event || !event->data) {
        return 0;
    }
    
    /* This is a placeholder for regex filtering */
    /* In a real implementation, you would use a regex library like PCRE */
    /* For now, we'll just do simple string matching */
    const char *pattern = (const char *)filter->user_data;
    if (!pattern) {
        return 0;
    }
    
    /* Simple wildcard matching */
    return wildcard_match(event->data, pattern) ? 1 : 0;
}

int file_filter_accept(logger_filter_t *filter, struct logger_event_t *event, void *user_data) {
    if (!filter || !event || !event->file) {
        return 0;
    }
    
    const char *filename_pattern = (const char *)filter->user_data;
    if (!filename_pattern) {
        return 0;
    }
    
    /* Accept if file matches the pattern */
    return wildcard_match(event->file, filename_pattern) ? 1 : 0;
}

int function_filter_accept(logger_filter_t *filter, struct logger_event_t *event, void *user_data) {
    if (!filter || !event || !event->func) {
        return 0;
    }
    
    const char *function_pattern = (const char *)filter->user_data;
    if (!function_pattern) {
        return 0;
    }
    
    /* Accept if function matches the pattern */
    return wildcard_match(event->func, function_pattern) ? 1 : 0;
}

/* Helper Functions */

static int wildcard_match(const char *str, const char *pattern) {
    if (!str || !pattern) {
        return 0;
    }
    
    /* Simple wildcard matching implementation */
    /* Supports '*' for any characters and '?' for single character */
    const char *str_ptr = str;
    const char *pattern_ptr = pattern;
    const char *str_backup = NULL;
    const char *pattern_backup = NULL;
    
    while (*str_ptr) {
        if (*pattern_ptr == '*') {
            /* Skip consecutive '*' */
            while (*pattern_ptr == '*') {
                pattern_ptr++;
            }
            
            /* If pattern ends with '*', match everything */
            if (*pattern_ptr == '\0') {
                return 1;
            }
            
            /* Save current positions for backtracking */
            str_backup = str_ptr;
            pattern_backup = pattern_ptr;
        } else if (*pattern_ptr == '?' || *pattern_ptr == *str_ptr) {
            /* Single character match */
            str_ptr++;
            pattern_ptr++;
        } else if (str_backup) {
            /* Backtrack to try different match */
            str_ptr = ++str_backup;
            pattern_ptr = pattern_backup;
        } else {
            /* No match */
            return 0;
        }
    }
    
    /* Skip remaining '*' in pattern */
    while (*pattern_ptr == '*') {
        pattern_ptr++;
    }
    
    /* Match if pattern is exhausted */
    return (*pattern_ptr == '\0') ? 1 : 0;
}
