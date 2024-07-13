#include "logger/logger.h"
#include "logger/appender_console.h"

#if defined(_WIN32) || defined(linux) || defined(__APPLE__)
#include <stdio.h>

void stdout_logger_writer(struct LOGGER_APPENDER *appender, int level, const char *buf, int len)
{
    // just ignore the level
    printf("%s", buf);
}

int logger_appender_console_get_size(void)
{
    return sizeof(struct LOGGER_APPENDER);
}

int logger_appender_console_init(struct LOGGER_APPENDER *appender)
{
    memset(appender, 0, sizeof(struct LOGGER_APPENDER));
    appender->writer = stdout_logger_writer;
    appender->level_mask = 0;
    return 0;
}

#endif
