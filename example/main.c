#if defined(WIN32)
#include <windows.h>
#else
#include <pthread.h>
#endif
#include "logger/logger.h"
#include "logger/layout_full.h"
#include "logger/appender_console.h"

static char _logbuf[1024];
static struct LOGGER _logger;
#if defined(WIN32)
static HANDLE _lock;
#else
static pthread_mutex_t _lock;
#endif

void _app_logger_lock_acquire(logger_lock_t lock)
{
#if defined(WIN32)
    WaitForSingleObject((HANDLE)lock, INFINITE);
#else
    pthread_mutex_lock((pthread_mutex_t *)lock);
#endif
}

void _app_logger_lock_release(logger_lock_t lock)
{
#if defined(WIN32)
    ReleaseMutex((HANDLE)lock);
#else
    pthread_mutex_unlock((pthread_mutex_t *)lock);
#endif
}

int main(void)
{
    struct LOGGER_CFG cfg = {0};
    struct LOGGER_LAYOUT layout;
    struct LOGGER_APPENDER appender;

#if !defined(WIN32)
    pthread_mutex_init(&_lock, NULL);
#endif

    cfg.buf = _logbuf;
    cfg.buf_size = sizeof(_logbuf);
    cfg.syscall_time = time;
    cfg.syscall_localtime = localtime;
#if defined(WIN32)
    cfg.lock = (logger_lock_t)_lock;
#else
    cfg.lock = (logger_lock_t)&_lock;
#endif
    cfg.acquire_lock = _app_logger_lock_acquire;
    cfg.release_lock = _app_logger_lock_release;

    // init logger, layout, appender
    logger_init(&_logger, &cfg);
    logger_set_default(&_logger);
    logger_layout_full_init(&layout);
    logger_appender_console_init(&appender);
    logger_layout_add_appender(&layout, &appender);
    logger_add_layout(&_logger, &layout);

    return 0;
}
