#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdbool.h>
#include "logger/logger.h"
#include "logger/appender_file.h"
#include "logger/osal.h"

/* File appender internal structure */
typedef struct {
    char filename[LOGGER_APPENDER_FILE_PATH_MAX];
    FILE *file_handle;
    size_t max_file_size;
    int max_files;
    char backup_path[LOGGER_APPENDER_FILE_PATH_MAX];
    size_t current_size;
    int current_file_index;
    bool enable_rotation;
    int rotation_mode;                  /* 0=size-based, 1=time-based */
    int rotation_interval_hours;        /* Time-based rotation interval */
    bool enable_compression;           /* Enable compression of rotated files */
    time_t last_rotation_time;         /* Last rotation time for time-based rotation */
} file_appender_data_t;

/* Forward declarations */
static int file_appender_write_func(logger_appender_t *appender, const struct logger_event_t *event);
static void check_and_rotate_file(file_appender_data_t *data);
static int create_backup_file(const char *original_path, const char *backup_path, int index);
static void cleanup_old_files(const char *backup_path, int max_files);
static int ensure_directory_exists(const char *path);

/* File appender write function */
static int file_appender_write_func(logger_appender_t *appender, const struct logger_event_t *event) {
    if (!appender || !event) {
        return -1;
    }
    
    file_appender_data_t *data = (file_appender_data_t *)appender->user_data;
    if (!data || !data->file_handle) {
        return -1;
    }
    
    /* Check if file rotation is needed */
    if (data->enable_rotation && data->current_size >= data->max_file_size) {
        check_and_rotate_file(data);
    }
    
    /* Write the log message */
    size_t written = fwrite(event->data, 1, event->msg_len, data->file_handle);
    if (written != event->msg_len) {
        return -1;
    }
    
    /* Flush to ensure data is written */
    fflush(data->file_handle);
    
    /* Update current size */
    data->current_size += written;
    
    return written;
}

/* Check and rotate file if needed */
static void check_and_rotate_file(file_appender_data_t *data) {
    if (!data->enable_rotation) {
        return;
    }
    
    bool should_rotate = false;
    
    /* Check rotation condition based on mode */
    if (data->rotation_mode == 0) {
        /* Size-based rotation */
        should_rotate = (data->current_size >= data->max_file_size);
    } else if (data->rotation_mode == 1) {
        /* Time-based rotation */
        time_t current_time = time(NULL);
        time_t interval_seconds = data->rotation_interval_hours * 3600;
        should_rotate = (current_time - data->last_rotation_time >= interval_seconds);
    }
    
    if (!should_rotate) {
        return;
    }
    
    /* Close current file */
    if (data->file_handle) {
        fclose(data->file_handle);
        data->file_handle = NULL;
    }
    
    /* Create backup file */
    char backup_filename[LOGGER_APPENDER_FILE_PATH_MAX];
    snprintf(backup_filename, sizeof(backup_filename), "%s.%d", 
             data->filename, data->current_file_index);
    
    if (create_backup_file(data->filename, backup_filename, data->current_file_index) == 0) {
        data->current_file_index++;
        
        /* Cleanup old files if we exceed max_files */
        if (data->current_file_index > data->max_files) {
            cleanup_old_files(data->backup_path, data->max_files);
            data->current_file_index = data->max_files;
        }
        
        /* Update last rotation time */
        data->last_rotation_time = time(NULL);
    }
    
    /* Reopen the original file */
    data->file_handle = fopen(data->filename, "a");
    if (data->file_handle) {
        data->current_size = 0;
    }
}

/* Create backup file */
static int create_backup_file(const char *original_path, const char *backup_path, int index) {
    FILE *src = fopen(original_path, "r");
    if (!src) {
        return -1;
    }
    
    FILE *dst = fopen(backup_path, "w");
    if (!dst) {
        fclose(src);
        return -1;
    }
    
    /* Copy file contents */
    char buffer[4096];
    size_t bytes_read;
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), src)) > 0) {
        fwrite(buffer, 1, bytes_read, dst);
    }
    
    fclose(src);
    fclose(dst);
    
    /* Clear the original file */
    src = fopen(original_path, "w");
    if (src) {
        fclose(src);
    }
    
    return 0;
}

/* Cleanup old backup files */
static void cleanup_old_files(const char *backup_path, int max_files) {
    char filename[LOGGER_APPENDER_FILE_PATH_MAX];
    
    /* Remove files beyond max_files limit */
    for (int i = max_files + 1; i <= max_files + 10; i++) {
        snprintf(filename, sizeof(filename), "%s.%d", backup_path, i);
        unlink(filename);
    }
}

/* Ensure directory exists */
static int ensure_directory_exists(const char *path) {
    char dir_path[LOGGER_APPENDER_FILE_PATH_MAX];
    char *last_slash = strrchr(path, '/');
    
    if (!last_slash) {
        return 0; /* No directory component */
    }
    
    /* Extract directory path */
    size_t dir_len = last_slash - path;
    if (dir_len >= sizeof(dir_path)) {
        return -1;
    }
    
    strncpy(dir_path, path, dir_len);
    dir_path[dir_len] = '\0';
    
    /* Check if directory exists */
    struct stat st;
    if (stat(dir_path, &st) == 0) {
        return S_ISDIR(st.st_mode) ? 0 : -1;
    }
    
    /* Create directory recursively */
    return mkdir(dir_path, 0755);
}

/* Public API Implementation */

int logger_appender_file_init(struct logger_appender_t *appender, const char *path) {
    if (!appender || !path) {
        return -1;
    }
    
    /* Allocate internal data */
    file_appender_data_t *data = logger_malloc(sizeof(file_appender_data_t));
    if (!data) {
        return -1;
    }
    
    memset(data, 0, sizeof(file_appender_data_t));
    
    /* Copy filename */
    strncpy(data->filename, path, sizeof(data->filename) - 1);
    data->filename[sizeof(data->filename) - 1] = '\0';
    
    /* Set default values */
    data->max_file_size = 10 * 1024 * 1024; /* 10MB default */
    data->max_files = 5; /* Keep 5 backup files */
    data->enable_rotation = true;
    data->current_file_index = 1;
    data->rotation_mode = 0; /* Default to size-based rotation */
    data->rotation_interval_hours = 24; /* Default to daily rotation */
    data->enable_compression = false; /* Default to no compression */
    data->last_rotation_time = time(NULL);
    
    /* Copy backup path */
    strncpy(data->backup_path, path, sizeof(data->backup_path) - 1);
    data->backup_path[sizeof(data->backup_path) - 1] = '\0';
    
    /* Ensure directory exists */
    if (ensure_directory_exists(path) != 0) {
        logger_free(data);
        return -1;
    }
    
    /* Open file for writing */
    data->file_handle = fopen(path, "a");
    if (!data->file_handle) {
        logger_free(data);
        return -1;
    }
    
    /* Get current file size */
    fseek(data->file_handle, 0, SEEK_END);
    data->current_size = ftell(data->file_handle);
    
    /* Initialize appender */
    appender->write = file_appender_write_func;
    appender->user_data = data;
    appender->buffer_size = LOGGER_DEFAULT_LINE_SIZE;
    
    return 0;
}

int logger_appender_file_config(struct logger_appender_t *appender, int cfg, ...) {
    if (!appender || !appender->user_data) {
        return -1;
    }
    
    file_appender_data_t *data = (file_appender_data_t *)appender->user_data;
    va_list args;
    va_start(args, cfg);
    
    switch (cfg) {
        case LOGGER_APPENDER_FILE_CFG_FILE_SIZE_LIMIT:
            data->max_file_size = va_arg(args, size_t);
            break;
            
        case LOGGER_APPENDER_FILE_CFG_BACKUP_PATH:
            {
                const char *backup_path = va_arg(args, const char *);
                if (backup_path) {
                    strncpy(data->backup_path, backup_path, sizeof(data->backup_path) - 1);
                    data->backup_path[sizeof(data->backup_path) - 1] = '\0';
                }
            }
            break;
            
        case LOGGER_APPENDER_FILE_CFG_MAX_FILES:
            data->max_files = va_arg(args, int);
            break;
            
        case LOGGER_APPENDER_FILE_CFG_ROTATION_MODE:
            data->rotation_mode = va_arg(args, int);
            break;
            
        case LOGGER_APPENDER_FILE_CFG_ROTATION_INTERVAL:
            data->rotation_interval_hours = va_arg(args, int);
            break;
            
        case LOGGER_APPENDER_FILE_CFG_ENABLE_COMPRESSION:
            data->enable_compression = va_arg(args, int) != 0;
            break;
            
        default:
            va_end(args);
            return -1;
    }
    
    va_end(args);
    return 0;
}

int logger_appender_file_deinit(struct logger_appender_t *appender) {
    if (!appender || !appender->user_data) {
        return -1;
    }
    
    file_appender_data_t *data = (file_appender_data_t *)appender->user_data;
    
    /* Close file handle */
    if (data->file_handle) {
        fclose(data->file_handle);
        data->file_handle = NULL;
    }
    
    /* Free internal data */
    logger_free(data);
    appender->user_data = NULL;
    
    return 0;
}

int logger_appender_file_get_size(void) {
    return sizeof(struct logger_appender_t);
}
