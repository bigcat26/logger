#include "logger/logger.h"
#include "logger/appender_console.h"

#if defined(_WIN32) || defined(linux) || defined(__APPLE__)
#include <stdio.h>

void stdout_logger_writer(struct logger_appender_t *appender, int level, const char *buf, int len)
{
    // just ignore the level
    printf("%s", buf);
}

int logger_appender_console_get_size(void)
{
    return sizeof(struct logger_appender_t);
}

int logger_appender_console_init(struct logger_appender_t *appender)
{
    memset(appender, 0, sizeof(struct logger_appender_t));
    appender->writer = stdout_logger_writer;
    appender->level_mask = 0;
    return 0;
}

#endif
