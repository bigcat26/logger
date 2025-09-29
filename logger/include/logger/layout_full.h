#ifndef __LOGGER_LAYOUT_FULL_H__
#define __LOGGER_LAYOUT_FULL_H__

#ifdef __cplusplus
extern "C" {
#endif

struct logger_layout_t;

int logger_layout_full_get_size();

int logger_layout_full_init(struct logger_layout_t *layout);

#ifdef __cplusplus
}
#endif

#endif // __LOGGER_LAYOUT_FULL_H__
