/*************************************************************************
    > File Name: wlog_profile.h
    > Author: wangkunyao
    > Mail: wangkyao1992@163.com 
    > Created Time: Mon 09 Oct 2017 11:12:12 PM CST
 ************************************************************************/

#ifndef _WLOG_PROFILE_H_
#define _WLOG_PROFILE_H_

#define WLOGSTRCASECMP(_s, _d)              (strncasecmp((_s), (_d), strlen(_d))==0)
#define WLOG_ARR_SIZE(_a)                   ((int)(sizeof(_a)/sizeof((_a)[0])))

#define WLOG_MIN(_a, _b)                    (((_a) > (_b))?(_b):(_a))
#define WLOG_STRCPY(_d, _s, _ds, _ss)       strncpy((_d), (_s), WLOG_MIN(_ds, _ss))

typedef enum
{
    INNER_DEBUG = 1,
    INNER_WARN,
    INNER_ERROR,
}wlog_profile_flag_t;

#define wp_debug(fmt, args...)   \
    wlog_profile_inner(INNER_DEBUG, __FILE__, __LINE__, fmt, ## args)
#define wp_warn(fmt, args...)   \
    wlog_profile_inner(INNER_WARN, __FILE__, __LINE__, fmt, ## args)
#define wp_error(fmt, args...)   \
    wlog_profile_inner(INNER_ERROR, __FILE__, __LINE__, fmt, ## args)

int wlog_profile_inner(wlog_profile_flag_t flag, const char *file, 
        const long line, const char *fmt, ...);

#endif
