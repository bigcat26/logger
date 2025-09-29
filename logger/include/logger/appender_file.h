#ifndef __LOGGER_APPENDER_FILE_H__
#define __LOGGER_APPENDER_FILE_H__

#ifdef __cplusplus
extern "C"
{
#endif

#define LOGGER_APPENDER_FILE_PATH_MAX               250

#define LOGGER_APPENDER_FILE_CFG_FILE_SIZE_LIMIT    1
#define LOGGER_APPENDER_FILE_CFG_BACKUP_PATH        2

int logger_appender_file_init(struct logger_appender_t *appender, const char *path);

int logger_appender_file_config(struct logger_appender_t *appender, int cfg, ...);

int logger_appender_file_deinit(struct logger_appender_t *appender);

int logger_appender_file_get_size(void);

#ifdef __cplusplus
}
#endif

#endif // __LOGGER_APPENDER_FILE_H__
