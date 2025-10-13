#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <stdbool.h>
#include <sys/stat.h>
#include "logger/logger.h"
#include "logger/error_handler.h"
#include "logger/osal.h"

/* Error handling internal structure */
typedef struct {
    int error_count;
    int warning_count;
    int last_error_code;
    char last_error_message[256];
    time_t last_error_time;
    bool enable_error_logging;
    bool enable_warning_logging;
} error_handler_data_t;

/* Global error handler instance */
static error_handler_data_t g_error_handler = {0};

/* Forward declarations */
static void error_handler_log_error(int error_code, const char *error_message);
static void error_handler_log_warning(int warning_code, const char *warning_message);
static const char *error_handler_get_error_string(int error_code);

/* Log error */
static void error_handler_log_error(int error_code, const char *error_message) {
    if (!g_error_handler.enable_error_logging) {
        return;
    }
    
    g_error_handler.error_count++;
    g_error_handler.last_error_code = error_code;
    g_error_handler.last_error_time = time(NULL);
    
    if (error_message) {
        strncpy(g_error_handler.last_error_message, error_message, 
                sizeof(g_error_handler.last_error_message) - 1);
        g_error_handler.last_error_message[sizeof(g_error_handler.last_error_message) - 1] = '\0';
    } else {
        strncpy(g_error_handler.last_error_message, error_handler_get_error_string(error_code),
                sizeof(g_error_handler.last_error_message) - 1);
        g_error_handler.last_error_message[sizeof(g_error_handler.last_error_message) - 1] = '\0';
    }
    
    /* Log to logger if available */
    LOGE("Logger Error [%d]: %s", error_code, g_error_handler.last_error_message);
}

/* Log warning */
static void error_handler_log_warning(int warning_code, const char *warning_message) {
    if (!g_error_handler.enable_warning_logging) {
        return;
    }
    
    g_error_handler.warning_count++;
    
    /* Log to logger if available */
    LOGW("Logger Warning [%d]: %s", warning_code, warning_message ? warning_message : "Unknown warning");
}

/* Get error string */
static const char *error_handler_get_error_string(int error_code) {
    switch (error_code) {
        case LOGGER_ERROR_INVALID_PARAMS:
            return "Invalid parameters";
        case LOGGER_ERROR_MEMORY_ALLOCATION:
            return "Memory allocation failed";
        case LOGGER_ERROR_FILE_OPERATION:
            return "File operation failed";
        case LOGGER_ERROR_NETWORK_OPERATION:
            return "Network operation failed";
        case LOGGER_ERROR_INVALID_STATE:
            return "Invalid state";
        case LOGGER_ERROR_NOT_INITIALIZED:
            return "Not initialized";
        case LOGGER_ERROR_ALREADY_INITIALIZED:
            return "Already initialized";
        case LOGGER_ERROR_BUFFER_OVERFLOW:
            return "Buffer overflow";
        case LOGGER_ERROR_TIMEOUT:
            return "Operation timeout";
        case LOGGER_ERROR_PERMISSION_DENIED:
            return "Permission denied";
        case LOGGER_ERROR_NOT_FOUND:
            return "Resource not found";
        case LOGGER_ERROR_INVALID_FORMAT:
            return "Invalid format";
        case LOGGER_ERROR_IO_ERROR:
            return "I/O error";
        case LOGGER_ERROR_THREAD_ERROR:
            return "Thread error";
        case LOGGER_ERROR_SYSTEM_ERROR:
            return "System error";
        default:
            return "Unknown error";
    }
}

/* Public API Implementation */

int logger_error_init(bool enable_error_logging, bool enable_warning_logging) {
    memset(&g_error_handler, 0, sizeof(g_error_handler));
    g_error_handler.enable_error_logging = enable_error_logging;
    g_error_handler.enable_warning_logging = enable_warning_logging;
    g_error_handler.last_error_time = time(NULL);
    
    return 0;
}

int logger_error_deinit(void) {
    memset(&g_error_handler, 0, sizeof(g_error_handler));
    return 0;
}

int logger_error_set_logging(bool enable_error_logging, bool enable_warning_logging) {
    g_error_handler.enable_error_logging = enable_error_logging;
    g_error_handler.enable_warning_logging = enable_warning_logging;
    return 0;
}

int logger_error_log(int error_code, const char *error_message) {
    error_handler_log_error(error_code, error_message);
    return 0;
}

int logger_warning_log(int warning_code, const char *warning_message) {
    error_handler_log_warning(warning_code, warning_message);
    return 0;
}

int logger_error_get_stats(int *error_count, int *warning_count, int *last_error_code, 
                          char *last_error_message, size_t message_size, time_t *last_error_time) {
    if (error_count) {
        *error_count = g_error_handler.error_count;
    }
    
    if (warning_count) {
        *warning_count = g_error_handler.warning_count;
    }
    
    if (last_error_code) {
        *last_error_code = g_error_handler.last_error_code;
    }
    
    if (last_error_message && message_size > 0) {
        strncpy(last_error_message, g_error_handler.last_error_message, message_size - 1);
        last_error_message[message_size - 1] = '\0';
    }
    
    if (last_error_time) {
        *last_error_time = g_error_handler.last_error_time;
    }
    
    return 0;
}

int logger_error_reset_stats(void) {
    g_error_handler.error_count = 0;
    g_error_handler.warning_count = 0;
    g_error_handler.last_error_code = 0;
    g_error_handler.last_error_message[0] = '\0';
    g_error_handler.last_error_time = time(NULL);
    
    return 0;
}

int logger_error_check_system_error(void) {
    int err = errno;
    if (err != 0) {
        logger_error_log(LOGGER_ERROR_SYSTEM_ERROR, strerror(err));
        return err;
    }
    return 0;
}

int logger_error_handle_memory_allocation(void *ptr, const char *function_name) {
    if (!ptr) {
        char error_msg[256];
        snprintf(error_msg, sizeof(error_msg), "Memory allocation failed in %s", 
                function_name ? function_name : "unknown function");
        logger_error_log(LOGGER_ERROR_MEMORY_ALLOCATION, error_msg);
        return -1;
    }
    return 0;
}

int logger_error_handle_file_operation(int result, const char *operation, const char *filename) {
    if (result < 0) {
        char error_msg[256];
        snprintf(error_msg, sizeof(error_msg), "File operation '%s' failed for '%s': %s", 
                operation ? operation : "unknown", 
                filename ? filename : "unknown file", 
                strerror(errno));
        logger_error_log(LOGGER_ERROR_FILE_OPERATION, error_msg);
        return -1;
    }
    return 0;
}

int logger_error_handle_network_operation(int result, const char *operation, const char *host) {
    if (result < 0) {
        char error_msg[256];
        snprintf(error_msg, sizeof(error_msg), "Network operation '%s' failed for '%s': %s", 
                operation ? operation : "unknown", 
                host ? host : "unknown host", 
                strerror(errno));
        logger_error_log(LOGGER_ERROR_NETWORK_OPERATION, error_msg);
        return -1;
    }
    return 0;
}

/* Error handling macros */
#define LOGGER_ERROR_CHECK_MEMORY(ptr, func) \
    logger_error_handle_memory_allocation(ptr, func)

#define LOGGER_ERROR_CHECK_FILE(result, operation, filename) \
    logger_error_handle_file_operation(result, operation, filename)

#define LOGGER_ERROR_CHECK_NETWORK(result, operation, host) \
    logger_error_handle_network_operation(result, operation, host)

#define LOGGER_ERROR_CHECK_SYSTEM() \
    logger_error_check_system_error()

/* Error recovery functions */
int logger_error_recover_from_memory_error(void) {
    /* Try to free some memory and retry */
    /* This is a placeholder for more sophisticated memory management */
    return 0;
}

int logger_error_recover_from_file_error(const char *filename) {
    /* Try to create directory or fix permissions */
    if (filename) {
        /* Check if directory exists and create if needed */
        char *last_slash = strrchr(filename, '/');
        if (last_slash) {
            char dir_path[256];
            size_t dir_len = last_slash - filename;
            if (dir_len < sizeof(dir_path)) {
                strncpy(dir_path, filename, dir_len);
                dir_path[dir_len] = '\0';
                
                /* Try to create directory */
                if (mkdir(dir_path, 0755) == 0) {
                    logger_warning_log(LOGGER_WARNING_DIRECTORY_CREATED, dir_path);
                    return 0;
                }
            }
        }
    }
    return -1;
}

int logger_error_recover_from_network_error(const char *host, int port) {
    /* Try to reconnect or use alternative endpoint */
    /* This is a placeholder for more sophisticated network recovery */
    if (host && port > 0) {
        logger_warning_log(LOGGER_WARNING_NETWORK_RECOVERY_ATTEMPTED, host);
        return 0;
    }
    return -1;
}
