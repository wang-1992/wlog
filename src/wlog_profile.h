/*************************************************************************
    > File Name: wlog_profile.h
    > Author: wangkunyao
    > Mail: wangkyao1992@163.com 
    > Created Time: Mon 09 Oct 2017 11:12:12 PM CST
 ************************************************************************/

#ifndef _WLOG_PROFILE_H_
#define _WLOG_PROFILE_H_

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
