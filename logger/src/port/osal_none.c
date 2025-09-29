#include "osal.h"
#include <time.h>
#include <pthread.h>

void logger_lock_new(logger_lock_t **lock) {
}

void logger_lock_acquire(logger_lock_t *lock) {
}

void logger_lock_release(logger_lock_t *lock) {
}

void logger_lock_free(logger_lock_t **lock) {
}

logger_time_t logger_gettime() {
    logger_time_t time = {
        .year = 1970,
        .month = 1,
        .day = 1,
        .hour = 0,
        .minute = 0,
        .second = 0,
    };
    return time;
}
