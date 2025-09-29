#ifndef __LOGGER_LAYOUT_SIMPLE_H__
#define __LOGGER_LAYOUT_SIMPLE_H__

#ifdef __cplusplus
extern "C" {
#endif

struct logger_layout_t;

int logger_layout_simple_get_size();

int logger_layout_simple_init(struct logger_layout_t *layout);

#ifdef __cplusplus
}
#endif

#endif // __LOGGER_LAYOUT_SIMPLE_H__
