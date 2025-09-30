#include "osal.h"
#include <time.h>
#include <pthread.h>
#include <stdint.h>

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

uint64_t logger_get_timestamp_ns() {
    time_t now = time(NULL);
    return (uint64_t)now * 1000000000ULL;
}
