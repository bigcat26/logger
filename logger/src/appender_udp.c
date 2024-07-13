#ifdef CONFIG_LOGGER_WITH_SYSLOG

#include <stdio.h>

#include "logger/logger.h"
#include "logger/appender_udp.h"

#if defined(_WIN32)
#include <winsock2.h>
#include <windows.h>
#elif defined(linux) || defined(__APPLE__)
#endif

struct UDP_LOGGER_APPENDER
{
    struct LOGGER_APPENDER appender;
    int fd;
    int addrlen;
    struct sockaddr addr;
};

static void logger_writer(struct LOGGER_APPENDER *appender, int level, const char *buf, int len)
{
    struct UDP_LOGGER_APPENDER *ua = (struct UDP_LOGGER_APPENDER *)appender;
    sendto(ua->fd, buf, len, 0, (struct sockaddr *)&ua->addr, ua->addrlen);
}

int logger_appender_udp_get_size(void)
{
    return sizeof(struct UDP_LOGGER_APPENDER);
}

int logger_appender_udp_deinit(struct LOGGER_APPENDER *appender)
{
    struct UDP_LOGGER_APPENDER *ua = (struct UDP_LOGGER_APPENDER *)appender;
    if (ua->fd >= 0)
    {
#ifdef WIN32
        closesocket(ua->fd);
#else
        close(ua->fd);
#endif
    }
    return 0;
}

int logger_appender_udp_init(struct LOGGER_APPENDER *appender, struct sockaddr *addr, int addrlen)
{
    struct UDP_LOGGER_APPENDER *ua = (struct UDP_LOGGER_APPENDER *)appender;
    memset(ua, 0, sizeof(struct UDP_LOGGER_APPENDER));
    ua->appender.writer = logger_writer;
    ua->fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (ua->fd >= 0) 
    {
        memcpy(&ua->addr, addr, addrlen);
        ua->addrlen = addrlen;
        return 0;
    }
    return -1;
}

#endif // CONFIG_LOGGER_WITH_SYSLOG
