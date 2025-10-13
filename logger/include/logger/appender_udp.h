#ifndef __LOGGER_APPENDER_UDP_H__
#define __LOGGER_APPENDER_UDP_H__

#include <sys/socket.h>
#include <time.h>

#ifdef __cplusplus
extern "C"
{
#endif

int logger_appender_udp_init(struct logger_appender_t *appender, struct sockaddr *addr, int addrlen);

int logger_appender_udp_set_max_packet_size(struct logger_appender_t *appender, int max_size);

int logger_appender_udp_set_retry_count(struct logger_appender_t *appender, int max_retries);

int logger_appender_udp_set_timeout(struct logger_appender_t *appender, int timeout_ms);

int logger_appender_udp_get_stats(struct logger_appender_t *appender, int *sent_packets, int *failed_packets);

int logger_appender_udp_set_max_backoff_delay(struct logger_appender_t *appender, int max_backoff_ms);

int logger_appender_udp_reset_error_state(struct logger_appender_t *appender);

int logger_appender_udp_get_error_state(struct logger_appender_t *appender, int *consecutive_failures, 
                                       int *backoff_delay_ms, time_t *last_failure_time);

int logger_appender_udp_deinit(struct logger_appender_t *appender);

int logger_appender_udp_get_size(void);

#ifdef __cplusplus
}
#endif

#endif // __LOGGER_APPENDER_UDP_H__
