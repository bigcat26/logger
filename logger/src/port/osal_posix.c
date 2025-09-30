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
