#include "logger/logger.h"

static char default_logger_buffer[1024];

int simple_logger_format_str(struct LOGGER_LAYOUT *layout, char *buf, int size, int level, const char *file, unsigned int line, const char *fmt, va_list ap);

int simple_logger_format_bin(struct LOGGER_LAYOUT *layout, char *buf, int size, int level, const char *file, unsigned int line, const void *dat, int len);

void stdout_logger_writer(struct LOGGER_APPENDER *appender, int level, const char *buf, int len);

static struct LOGGER_APPENDER console_appender = {
    NULL,
    0,
    stdout_logger_writer};

static struct LOGGER_LAYOUT default_logger_layout = {
    NULL,
    NULL,
    simple_logger_format_str,
    simple_logger_format_bin,
    &console_appender};

struct LOGGER default_logger = {
    LOGGER_LEVEL_ALL,
    {
        default_logger_buffer,
        sizeof(default_logger_buffer),
        NULL,
        time,
        localtime,
        NULL,
        NULL,
    },
    &default_logger_layout};
