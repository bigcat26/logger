#ifndef __LOGGER_OSAL_H__
#define __LOGGER_OSAL_H__

#include <stdint.h>

typedef void logger_lock_t;

typedef struct logger_time {
    int year;
    int month;
    int day;
    int hour;
    int minute;
    int second;
} logger_time_t;

void *logger_malloc(size_t size);

void logger_free(void *ptr);

void logger_lock_new(logger_lock_t **lock);

void logger_lock_acquire(logger_lock_t *lock);

void logger_lock_release(logger_lock_t *lock);

void logger_lock_free(logger_lock_t **lock);

uint64_t logger_get_timestamp_ns();

#endif
