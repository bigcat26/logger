#include "logger/logger.h"
#include "logger/appender_console.h"

#if defined(_WIN32) || defined(linux) || defined(__APPLE__)
#include <stdio.h>

int stdout_logger_writer(struct logger_appender_t *appender, const struct logger_event_t *event)
{
    if (!appender || !event) {
        return -1;
    }
    
    /* Simple implementation - just print the message */
    printf("%s", event->data);
    fflush(stdout);
    
    return event->msg_len;
}

int logger_appender_console_get_size(void)
{
    return sizeof(struct logger_appender_t);
}

int logger_appender_console_init(struct logger_appender_t *appender)
{
    memset(appender, 0, sizeof(struct logger_appender_t));
    appender->write = stdout_logger_writer;
    return 0;
}

#endif
