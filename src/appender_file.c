#ifdef CONFIG_MODULE_LOGGER_WITH_FILE
#include <stdio.h>

#include "logger/logger.h"
#include "logger/appender_file.h"

struct FILE_LOGGER_APPENDER
{
    struct LOGGER_APPENDER appender;
    FILE *fp;
    char path[LOGGER_APPENDER_FILE_PATH_MAX + 4];
    char bakpath[LOGGER_APPENDER_FILE_PATH_MAX + 4];
    unsigned int file_size_now;
    unsigned int file_size_limit;
};

static void logger_writer(struct LOGGER_APPENDER *appender, int level, const char *buf, int len)
{
    struct FILE_LOGGER_APPENDER *fa = (struct FILE_LOGGER_APPENDER *)appender;
    if (fa->file_size_now + len >= fa->file_size_limit) {
        fclose(fa->fp);
        if (strlen(fa->bakpath)) {
            unlink(fa->bakpath);
            rename(fa->path, fa->bakpath);
        } else {
            unlink(fa->path);
        }
        fa->fp = fopen(fa->path, "a+");
        fa->file_size_now = 0;
    }
    if (NULL != fa->fp) {
        fa->file_size_now += (unsigned int)fwrite(buf, 1, len, fa->fp);
        fflush(fa->fp);
    }
}

int logger_appender_file_config(struct LOGGER_APPENDER *appender, int cfg, ...)
{
    int res;
    va_list ap;
    struct FILE_LOGGER_APPENDER *fa = (struct FILE_LOGGER_APPENDER *)appender;

    va_start(ap, cfg);
    switch (cfg) {
    case LOGGER_APPENDER_FILE_CFG_FILE_SIZE_LIMIT:
        fa->file_size_limit = va_arg(ap, unsigned int);
        res = 0;
        break;
    case LOGGER_APPENDER_FILE_CFG_BACKUP_PATH:
        memset(fa->bakpath, 0, LOGGER_APPENDER_FILE_PATH_MAX + 4);
        strncpy(fa->bakpath, va_arg(ap, char *), LOGGER_APPENDER_FILE_PATH_MAX);
        res = 0;
        break;
    default:
        res = -1;
    }
    va_end(ap);
    return res;
}

int logger_appender_file_get_size(void)
{
    return sizeof(struct FILE_LOGGER_APPENDER);
}

int logger_appender_file_deinit(struct LOGGER_APPENDER *appender)
{
    struct FILE_LOGGER_APPENDER *fa = (struct FILE_LOGGER_APPENDER *)appender;
    if (fa->fp != NULL) {
        fclose(fa->fp);
        fa->fp = NULL;
    }
    return 0;
}

int logger_appender_file_init(struct LOGGER_APPENDER *appender, const char *path)
{
    struct FILE_LOGGER_APPENDER *fa = (struct FILE_LOGGER_APPENDER *)appender;
    memset(fa, 0, sizeof(struct FILE_LOGGER_APPENDER));
    fa->appender.writer = logger_writer;
    strncpy(fa->path, path, LOGGER_APPENDER_FILE_PATH_MAX);
    // default log file size limited to 1M
    fa->file_size_limit = 1 * 1024 * 1024;
    fa->fp = fopen(path, "a+");
    if (NULL != fa->fp) {
        // get file length
        fseek(fa->fp, 0, SEEK_END);
        fa->file_size_now = ftell(fa->fp);
        return 0;
    }
    // open file failed
    return -1;
}

#endif // CONFIG_MODULE_LOGGER_WITH_FILE
