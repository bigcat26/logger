#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <stdbool.h>
#include "logger/logger.h"
#include "logger/appender_udp.h"
#include "logger/osal.h"

/* UDP appender internal structure */
typedef struct {
    int socket_fd;
    struct sockaddr_in server_addr;
    char host[64];
    int port;
    int max_packet_size;
    bool enable_checksum;
    bool connected;
    int retry_count;
    int max_retries;
    int timeout_ms;
    int consecutive_failures;        /* Track consecutive failures */
    time_t last_success_time;         /* Last successful send time */
    time_t last_failure_time;         /* Last failure time */
    int backoff_delay_ms;             /* Current backoff delay */
    int max_backoff_delay_ms;         /* Maximum backoff delay */
} udp_appender_data_t;

/* Forward declarations */
static int udp_appender_write_func(logger_appender_t *appender, const struct logger_event_t *event);
static int udp_appender_connect(udp_appender_data_t *data);
static int udp_appender_send(udp_appender_data_t *data, const char *data_ptr, size_t len);
static void udp_appender_disconnect(udp_appender_data_t *data);
static int create_udp_socket(void);
static int resolve_hostname(const char *host, struct in_addr *addr);

/* UDP appender write function */
static int udp_appender_write_func(logger_appender_t *appender, const struct logger_event_t *event) {
    if (!appender || !event) {
        return -1;
    }
    
    udp_appender_data_t *data = (udp_appender_data_t *)appender->user_data;
    if (!data) {
        return -1;
    }
    
    /* Ensure connection is established */
    if (!data->connected) {
        if (udp_appender_connect(data) != 0) {
            return -1;
        }
    }
    
    /* Send the log message */
    int result = udp_appender_send(data, event->data, event->msg_len);
    
    /* Handle connection errors */
    if (result < 0 && data->retry_count < data->max_retries) {
        data->retry_count++;
        udp_appender_disconnect(data);
        
        /* Retry connection */
        if (udp_appender_connect(data) == 0) {
            result = udp_appender_send(data, event->data, event->msg_len);
        }
    }
    
    return result;
}

/* Connect to UDP server */
static int udp_appender_connect(udp_appender_data_t *data) {
    if (data->connected) {
        return 0;
    }
    
    /* Create socket if not exists */
    if (data->socket_fd < 0) {
        data->socket_fd = create_udp_socket();
        if (data->socket_fd < 0) {
            return -1;
        }
    }
    
    /* Resolve hostname */
    struct in_addr addr;
    if (resolve_hostname(data->host, &addr) != 0) {
        return -1;
    }
    
    /* Setup server address */
    memset(&data->server_addr, 0, sizeof(data->server_addr));
    data->server_addr.sin_family = AF_INET;
    data->server_addr.sin_addr = addr;
    data->server_addr.sin_port = htons(data->port);
    
    data->connected = true;
    data->retry_count = 0;
    
    return 0;
}

/* Send data via UDP */
static int udp_appender_send(udp_appender_data_t *data, const char *data_ptr, size_t len) {
    if (!data->connected || data->socket_fd < 0) {
        return -1;
    }
    
    /* Check if we should apply backoff delay */
    if (data->consecutive_failures > 0 && data->backoff_delay_ms > 0) {
        time_t current_time = time(NULL);
        if (current_time - data->last_failure_time < data->backoff_delay_ms / 1000) {
            return -1; /* Still in backoff period */
        }
    }
    
    /* Truncate data if it exceeds max packet size */
    size_t send_len = len;
    if (send_len > data->max_packet_size) {
        send_len = data->max_packet_size;
    }
    
    /* Send the data */
    ssize_t sent = sendto(data->socket_fd, data_ptr, send_len, 0,
                         (struct sockaddr *)&data->server_addr, 
                         sizeof(data->server_addr));
    
    if (sent < 0) {
        /* Handle specific errors */
        data->consecutive_failures++;
        data->last_failure_time = time(NULL);
        
        switch (errno) {
            case ECONNREFUSED:
            case EHOSTUNREACH:
            case ENETUNREACH:
                data->connected = false;
                /* Exponential backoff for connection errors */
                data->backoff_delay_ms = (data->backoff_delay_ms == 0) ? 1000 : 
                                       (data->backoff_delay_ms * 2);
                if (data->backoff_delay_ms > data->max_backoff_delay_ms) {
                    data->backoff_delay_ms = data->max_backoff_delay_ms;
                }
                break;
            case EAGAIN:
                /* Temporary error, don't disconnect */
                data->backoff_delay_ms = 100; /* Short delay for temporary errors */
                break;
            case EMSGSIZE:
                /* Message too large, don't retry */
                data->consecutive_failures = 0;
                break;
            default:
                data->connected = false;
                data->backoff_delay_ms = (data->backoff_delay_ms == 0) ? 1000 : 
                                       (data->backoff_delay_ms * 2);
                if (data->backoff_delay_ms > data->max_backoff_delay_ms) {
                    data->backoff_delay_ms = data->max_backoff_delay_ms;
                }
                break;
        }
        return -1;
    }
    
    /* Success - reset failure tracking */
    data->consecutive_failures = 0;
    data->last_success_time = time(NULL);
    data->backoff_delay_ms = 0;
    
    return sent;
}

/* Disconnect from UDP server */
static void udp_appender_disconnect(udp_appender_data_t *data) {
    if (data->socket_fd >= 0) {
        close(data->socket_fd);
        data->socket_fd = -1;
    }
    data->connected = false;
}

/* Create UDP socket */
static int create_udp_socket(void) {
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        return -1;
    }
    
    /* Set socket options for better reliability */
    int opt = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    /* Set timeout for send operations */
    struct timeval timeout;
    timeout.tv_sec = 5;  /* 5 seconds timeout */
    timeout.tv_usec = 0;
    setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
    
    return sockfd;
}

/* Resolve hostname to IP address */
static int resolve_hostname(const char *host, struct in_addr *addr) {
    if (!host || !addr) {
        return -1;
    }
    
    /* Try to parse as IP address first */
    if (inet_aton(host, addr) == 1) {
        return 0;
    }
    
    /* TODO: Implement DNS resolution if needed */
    /* For now, assume it's an IP address */
    return -1;
}

/* Public API Implementation */

int logger_appender_udp_init(struct logger_appender_t *appender, struct sockaddr *addr, int addrlen) {
    if (!appender || !addr || addrlen <= 0) {
        return -1;
    }
    
    /* Allocate internal data */
    udp_appender_data_t *data = logger_malloc(sizeof(udp_appender_data_t));
    if (!data) {
        return -1;
    }
    
    memset(data, 0, sizeof(udp_appender_data_t));
    
    /* Copy address information */
    if (addr->sa_family == AF_INET) {
        struct sockaddr_in *addr_in = (struct sockaddr_in *)addr;
        data->server_addr = *addr_in;
        
        /* Extract host and port */
        inet_ntop(AF_INET, &addr_in->sin_addr, data->host, sizeof(data->host));
        data->port = ntohs(addr_in->sin_port);
    } else {
        logger_free(data);
        return -1; /* Unsupported address family */
    }
    
    /* Set default values */
    data->socket_fd = -1;
    data->max_packet_size = 1024; /* 1KB default */
    data->enable_checksum = true;
    data->connected = false;
    data->retry_count = 0;
    data->max_retries = 3;
    data->timeout_ms = 5000; /* 5 seconds */
    data->consecutive_failures = 0;
    data->last_success_time = 0;
    data->last_failure_time = 0;
    data->backoff_delay_ms = 0;
    data->max_backoff_delay_ms = 30000; /* 30 seconds max backoff */
    
    /* Initialize appender */
    appender->write = udp_appender_write_func;
    appender->user_data = data;
    appender->buffer_size = LOGGER_DEFAULT_LINE_SIZE;
    
    return 0;
}

int logger_appender_udp_get_size(void) {
    return sizeof(struct logger_appender_t);
}

/* Additional helper functions for UDP appender configuration */

int logger_appender_udp_set_max_packet_size(struct logger_appender_t *appender, int max_size) {
    if (!appender || !appender->user_data || max_size <= 0) {
        return -1;
    }
    
    udp_appender_data_t *data = (udp_appender_data_t *)appender->user_data;
    data->max_packet_size = max_size;
    
    return 0;
}

int logger_appender_udp_set_retry_count(struct logger_appender_t *appender, int max_retries) {
    if (!appender || !appender->user_data || max_retries < 0) {
        return -1;
    }
    
    udp_appender_data_t *data = (udp_appender_data_t *)appender->user_data;
    data->max_retries = max_retries;
    
    return 0;
}

int logger_appender_udp_set_timeout(struct logger_appender_t *appender, int timeout_ms) {
    if (!appender || !appender->user_data || timeout_ms <= 0) {
        return -1;
    }
    
    udp_appender_data_t *data = (udp_appender_data_t *)appender->user_data;
    data->timeout_ms = timeout_ms;
    
    /* Update socket timeout if connected */
    if (data->connected && data->socket_fd >= 0) {
        struct timeval timeout;
        timeout.tv_sec = timeout_ms / 1000;
        timeout.tv_usec = (timeout_ms % 1000) * 1000;
        setsockopt(data->socket_fd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
    }
    
    return 0;
}

int logger_appender_udp_get_stats(struct logger_appender_t *appender, int *sent_packets, int *failed_packets) {
    if (!appender || !appender->user_data || !sent_packets || !failed_packets) {
        return -1;
    }
    
    udp_appender_data_t *data = (udp_appender_data_t *)appender->user_data;
    
    /* TODO: Implement packet statistics tracking */
    *sent_packets = 0;
    *failed_packets = data->retry_count;
    
    return 0;
}

int logger_appender_udp_set_max_backoff_delay(struct logger_appender_t *appender, int max_backoff_ms) {
    if (!appender || !appender->user_data || max_backoff_ms <= 0) {
        return -1;
    }
    
    udp_appender_data_t *data = (udp_appender_data_t *)appender->user_data;
    data->max_backoff_delay_ms = max_backoff_ms;
    
    return 0;
}

int logger_appender_udp_reset_error_state(struct logger_appender_t *appender) {
    if (!appender || !appender->user_data) {
        return -1;
    }
    
    udp_appender_data_t *data = (udp_appender_data_t *)appender->user_data;
    data->consecutive_failures = 0;
    data->backoff_delay_ms = 0;
    data->last_failure_time = 0;
    
    return 0;
}

int logger_appender_udp_get_error_state(struct logger_appender_t *appender, int *consecutive_failures, 
                                       int *backoff_delay_ms, time_t *last_failure_time) {
    if (!appender || !appender->user_data) {
        return -1;
    }
    
    udp_appender_data_t *data = (udp_appender_data_t *)appender->user_data;
    
    if (consecutive_failures) {
        *consecutive_failures = data->consecutive_failures;
    }
    
    if (backoff_delay_ms) {
        *backoff_delay_ms = data->backoff_delay_ms;
    }
    
    if (last_failure_time) {
        *last_failure_time = data->last_failure_time;
    }
    
    return 0;
}
