#ifndef __LOGGER_ERROR_HANDLER_H__
#define __LOGGER_ERROR_HANDLER_H__

#include <stdbool.h>
#include <stddef.h>
#include <time.h>

#ifdef __cplusplus
extern "C"
{
#endif

/* Error codes */
#define LOGGER_ERROR_INVALID_PARAMS        -1
#define LOGGER_ERROR_MEMORY_ALLOCATION     -2
#define LOGGER_ERROR_FILE_OPERATION         -3
#define LOGGER_ERROR_NETWORK_OPERATION     -4
#define LOGGER_ERROR_INVALID_STATE         -5
#define LOGGER_ERROR_NOT_INITIALIZED       -6
#define LOGGER_ERROR_ALREADY_INITIALIZED   -7
#define LOGGER_ERROR_BUFFER_OVERFLOW       -8
#define LOGGER_ERROR_TIMEOUT               -9
#define LOGGER_ERROR_PERMISSION_DENIED     -10
#define LOGGER_ERROR_NOT_FOUND             -11
#define LOGGER_ERROR_INVALID_FORMAT        -12
#define LOGGER_ERROR_IO_ERROR              -13
#define LOGGER_ERROR_THREAD_ERROR          -14
#define LOGGER_ERROR_SYSTEM_ERROR          -15

/* Warning codes */
#define LOGGER_WARNING_DIRECTORY_CREATED   1001
#define LOGGER_WARNING_NETWORK_RECOVERY_ATTEMPTED 1002
#define LOGGER_WARNING_FILE_ROTATION      1003
#define LOGGER_WARNING_BUFFER_FULL        1004
#define LOGGER_WARNING_TIMEOUT_OCCURRED   1005

/* Error handling API */

/**
 * Initialize error handler
 * @param enable_error_logging Enable error logging
 * @param enable_warning_logging Enable warning logging
 * @return 0 on success, negative value on error
 */
int logger_error_init(bool enable_error_logging, bool enable_warning_logging);

/**
 * Deinitialize error handler
 * @return 0 on success, negative value on error
 */
int logger_error_deinit(void);

/**
 * Set logging options
 * @param enable_error_logging Enable error logging
 * @param enable_warning_logging Enable warning logging
 * @return 0 on success, negative value on error
 */
int logger_error_set_logging(bool enable_error_logging, bool enable_warning_logging);

/**
 * Log an error
 * @param error_code Error code
 * @param error_message Error message (can be NULL)
 * @return 0 on success, negative value on error
 */
int logger_error_log(int error_code, const char *error_message);

/**
 * Log a warning
 * @param warning_code Warning code
 * @param warning_message Warning message (can be NULL)
 * @return 0 on success, negative value on error
 */
int logger_warning_log(int warning_code, const char *warning_message);

/**
 * Get error statistics
 * @param error_count Pointer to store error count
 * @param warning_count Pointer to store warning count
 * @param last_error_code Pointer to store last error code
 * @param last_error_message Buffer to store last error message
 * @param message_size Size of the message buffer
 * @param last_error_time Pointer to store last error time
 * @return 0 on success, negative value on error
 */
int logger_error_get_stats(int *error_count, int *warning_count, int *last_error_code, 
                          char *last_error_message, size_t message_size, time_t *last_error_time);

/**
 * Reset error statistics
 * @return 0 on success, negative value on error
 */
int logger_error_reset_stats(void);

/**
 * Check for system errors (errno)
 * @return errno value if error occurred, 0 if no error
 */
int logger_error_check_system_error(void);

/**
 * Handle memory allocation errors
 * @param ptr Pointer to check
 * @param function_name Function name where allocation occurred
 * @return 0 on success, negative value on error
 */
int logger_error_handle_memory_allocation(void *ptr, const char *function_name);

/**
 * Handle file operation errors
 * @param result Result of file operation
 * @param operation Operation name
 * @param filename Filename
 * @return 0 on success, negative value on error
 */
int logger_error_handle_file_operation(int result, const char *operation, const char *filename);

/**
 * Handle network operation errors
 * @param result Result of network operation
 * @param operation Operation name
 * @param host Host name
 * @return 0 on success, negative value on error
 */
int logger_error_handle_network_operation(int result, const char *operation, const char *host);

/* Error recovery functions */

/**
 * Recover from memory error
 * @return 0 on success, negative value on error
 */
int logger_error_recover_from_memory_error(void);

/**
 * Recover from file error
 * @param filename Filename that caused error
 * @return 0 on success, negative value on error
 */
int logger_error_recover_from_file_error(const char *filename);

/**
 * Recover from network error
 * @param host Host that caused error
 * @param port Port that caused error
 * @return 0 on success, negative value on error
 */
int logger_error_recover_from_network_error(const char *host, int port);

/* Error handling macros */
#define LOGGER_ERROR_CHECK_MEMORY(ptr, func) \
    logger_error_handle_memory_allocation(ptr, func)

#define LOGGER_ERROR_CHECK_FILE(result, operation, filename) \
    logger_error_handle_file_operation(result, operation, filename)

#define LOGGER_ERROR_CHECK_NETWORK(result, operation, host) \
    logger_error_handle_network_operation(result, operation, host)

#define LOGGER_ERROR_CHECK_SYSTEM() \
    logger_error_check_system_error()

#ifdef __cplusplus
}
#endif

#endif // __LOGGER_ERROR_HANDLER_H__

