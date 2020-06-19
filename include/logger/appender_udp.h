#ifndef __LOGGER_APPENDER_UDP_H__
#define __LOGGER_APPENDER_UDP_H__

#ifdef __cplusplus
extern "C"
{
#endif

int logger_appender_udp_init(struct LOGGER_APPENDER *appender, struct sockaddr *addr, int addrlen);

int logger_appender_udp_get_size(void);

#ifdef __cplusplus
}
#endif

#endif // __LOGGER_APPENDER_UDP_H__
