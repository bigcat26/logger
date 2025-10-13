#ifndef __LOGGER_LAYOUT_SYSLOG_H__
#define __LOGGER_LAYOUT_SYSLOG_H__

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif

#define SYSLOG_FACILITY_KERN        0       /// KERNEL MESSAGES    
#define SYSLOG_FACILITY_USER        1       /// USER-LEVEL MESSAGES (DEFAULT)    
#define SYSLOG_FACILITY_MAIL        2       /// MAIL SYSTEM
#define SYSLOG_FACILITY_DAEMON      3       /// SYSTEM DAEMONS
#define SYSLOG_FACILITY_AUTH        4       /// SECURITY/AUTHORIZATION MESSAGES
#define SYSLOG_FACILITY_SYSLOGD     5       /// MESSAGES GENERATED INTERNALLY BY SYSLOGD
#define SYSLOG_FACILITY_LPR         6       /// LINE PRINTER SUBSYSTEM
#define SYSLOG_FACILITY_NEWS        7       /// NETWORK NEWS SUSSYSTEM
#define SYSLOG_FACILITY_UUCP        8       /// UUCP SUBSYSTEM
#define SYSLOG_FACILITY_CLOCK       9       /// CLOCK DAEMON
#define SYSLOG_FACILITY_AUTHPRI     10      /// SECURITY/AUTHORIZATION MESSAGES
#define SYSLOG_FACILITY_FTP         11      /// FTP DAEMON
#define SYSLOG_FACILITY_NTP         12      
#define SYSLOG_FACILITY_LOG_AUDIT   13      
#define SYSLOG_FACILITY_LOG_ALERT   14      
#define SYSLOG_FACILITY_CRON        15      
#define SYSLOG_FACILITY_LOCAL0      16      /// RESERVED FOR LOCAL USE
#define SYSLOG_FACILITY_LOCAL1      17      /// RESERVED FOR LOCAL USE
#define SYSLOG_FACILITY_LOCAL2      18      /// RESERVED FOR LOCAL USE
#define SYSLOG_FACILITY_LOCAL3      19      /// RESERVED FOR LOCAL USE
#define SYSLOG_FACILITY_LOCAL4      20      /// RESERVED FOR LOCAL USE
#define SYSLOG_FACILITY_LOCAL5      21      /// RESERVED FOR LOCAL USE
#define SYSLOG_FACILITY_LOCAL6      22      /// RESERVED FOR LOCAL USE
#define SYSLOG_FACILITY_LOCAL7      23      /// RESERVED FOR LOCAL USE


#define SYSLOG_PRIORITY_EMERG       0       /* system is unusable */
#define SYSLOG_PRIORITY_ALERT       1       /* action must be taken immediately */     
#define SYSLOG_PRIORITY_CRIT        2       /* critical conditions */    
#define SYSLOG_PRIORITY_ERR         3       /* error conditions */
#define SYSLOG_PRIORITY_WARNING     4       /* warning conditions */    
#define SYSLOG_PRIORITY_NOTICE      5       /* normal but significant condition */    
#define SYSLOG_PRIORITY_INFO        6       /* informational */    
#define SYSLOG_PRIORITY_DEBUG       7       /* debug-level messages */    

int logger_layout_syslog_get_size();

int logger_layout_syslog_init(struct logger_layout_t *layout, int facility, const char *tag, int pid);

int logger_layout_syslog_set_facility(struct logger_layout_t *layout, int facility);

int logger_layout_syslog_set_tag(struct logger_layout_t *layout, const char *tag);

int logger_layout_syslog_set_pid(struct logger_layout_t *layout, int pid);

int logger_layout_syslog_set_options(struct logger_layout_t *layout, bool include_timestamp,
                                   bool include_hostname, bool include_tag, bool include_pid);

int logger_layout_syslog_deinit(struct logger_layout_t *layout);

int logger_create_syslog_message(char *buffer, size_t size, log_level_t level, int facility,
                                const char *tag, int pid, const char *message);

int logger_layout_syslog_test(void);

#ifdef __cplusplus
}
#endif

#endif // __LOGGER_LAYOUT_SYSLOG_H__
