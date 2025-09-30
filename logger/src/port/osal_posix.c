#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

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
    
#ifdef __APPLE__
    // macOS 使用 localtime_r 可能有问题，使用 localtime 并复制结构体
    struct tm *tm_ptr = localtime(&now);
    if (tm_ptr) {
        tm = *tm_ptr;
    } else {
        // 如果获取时间失败，返回默认时间
        tm.tm_year = 70;  // 1970
        tm.tm_mon = 0;    // 1月
        tm.tm_mday = 1;
        tm.tm_hour = 0;
        tm.tm_min = 0;
        tm.tm_sec = 0;
    }
#else
    // Linux 和其他 POSIX 系统使用 localtime_r
    localtime_r(&now, &tm);
#endif
    
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

uint64_t logger_get_timestamp_ns() {
    struct timespec ts;
#ifdef __APPLE__
    // macOS 使用 clock_gettime
    if (clock_gettime(CLOCK_REALTIME, &ts) == 0) {
        return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
    }
#else
    // Linux 和其他 POSIX 系统
    if (clock_gettime(CLOCK_REALTIME, &ts) == 0) {
        return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
    }
#endif
    // 如果 clock_gettime 失败，使用 time() 函数
    time_t now = time(NULL);
    return (uint64_t)now * 1000000000ULL;
}
