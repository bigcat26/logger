#include "osal.h"
#include <time.h>
#include <pthread.h>
#include <stdint.h>

void logger_lock_new(logger_lock_t **lock) {
}

void logger_lock_acquire(logger_lock_t *lock) {
}

void logger_lock_release(logger_lock_t *lock) {
}

void logger_lock_free(logger_lock_t **lock) {
}

uint64_t logger_get_timestamp_ns() {
    time_t now = time(NULL);
    return (uint64_t)now * 1000000000ULL;
}
