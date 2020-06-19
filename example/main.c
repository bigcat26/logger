#include <stdio.h>
#include "logger/logger.h"

static char _logbuf[1024];
static struct LOGGER _logger;

int main(void)
{
    struct LOGGER_CFG cfg = {0};
    struct LOGGER_LAYOUT layout;
    struct LOGGER_APPENDER appender;

    cfg.buf = _logbuf;
    cfg.buf_size = sizeof(_logbuf);
    cfg.syscall_time = time;
    cfg.syscall_localtime = localtime;

    // init logger, layout, appender
    logger_init(&_logger, &cfg);
    logger_set_default(&_logger);
    logger_layout_full_init(&layout);
    logger_appender_console_init(&appender);
    logger_layout_add_appender(&layout, &appender);
    logger_add_layout(&_logger, &layout);

    LOGLV("hello, world");
    LOGLD("hello, world");
    LOGLI("hello, world");
    LOGLW("hello, world");
    LOGLE("hello, world");
    LOGLF("hello, world");

	return 0;
}
