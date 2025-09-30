#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#ifdef __linux__
#include <malloc.h>
#elif __APPLE__
#include <malloc/malloc.h>
#endif

#include "logger/osal.h"

void *logger_malloc(size_t size) {
    return malloc(size);
}

void logger_free(void *ptr) {
    free(ptr);
}

void logger_lock_new(logger_lock_t **lock) {
    pthread_mutex_t *mutex = malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(mutex, NULL);
    *lock = mutex;
}

void logger_lock_acquire(logger_lock_t *lock) {
    pthread_mutex_t *mutex = (pthread_mutex_t *)lock;
    pthread_mutex_lock(mutex);
}

void logger_lock_release(logger_lock_t *lock) {
    pthread_mutex_t *mutex = (pthread_mutex_t *)lock;
    pthread_mutex_unlock(mutex);
}

void logger_lock_free(logger_lock_t **lock) {
    if (*lock) {
        pthread_mutex_t *mutex = (pthread_mutex_t *)*lock;
        pthread_mutex_destroy(mutex);
        free(*lock);
        *lock = NULL;
    }
}

logger_time_t logger_gettime() {
    time_t now = time(NULL);
    struct tm tm;
    localtime_r(&now, &tm);
    logger_time_t time = {
        .year = tm.tm_year + 1900,
        .month = tm.tm_mon + 1,
        .day = tm.tm_mday,
        .hour = tm.tm_hour,
        .minute = tm.tm_min,
        .second = tm.tm_sec,
    };
    return time;
}
