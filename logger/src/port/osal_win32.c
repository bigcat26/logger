#include "osal.h"
#include <time.h>
#include <pthread.h>

void logger_lock_new(logger_lock_t **lock) {
    HANDLE handle = CreateMutex(NULL, FALSE, NULL);
    *lock = handle;
}

void logger_lock_acquire(logger_lock_t *lock) {
    WaitForSingleObject((HANDLE)lock, INFINITE);
}

void logger_lock_release(logger_lock_t *lock) {
    ReleaseMutex((HANDLE)lock);
}

void logger_lock_free(logger_lock_t **lock) {
    CloseHandle((HANDLE)*lock);
    *lock = NULL;
}

logger_time_t logger_gettime() {
    time_t now = time(NULL);
    struct tm tm;
    localtime_r(&now, &tm);
    logger_time_t time = {
        .year = tm->tm_year + 1900,
        .month = tm->tm_mon + 1,
        .day = tm->tm_mday,
        .hour = tm->tm_hour,
        .minute = tm->tm_min,
        .second = tm->tm_sec,
    };
    return time;
}
