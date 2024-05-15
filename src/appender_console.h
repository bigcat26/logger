#ifndef __LOGGER_APPENDER_CONSOLE_H__
#define __LOGGER_APPENDER_CONSOLE_H__

#ifdef __cplusplus
extern "C"
{
#endif

int logger_appender_console_init(struct LOGGER_APPENDER *appender);

int logger_appender_console_get_size(void);

#ifdef __cplusplus
}
#endif

#endif // __LOGGER_APPENDER_CONSOLE_H__
