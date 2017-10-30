/*************************************************************************
    > File Name: wlog.h
    > Author: wangkunyao
    > Mail: wangkyao1992@163.com 
    > Created Time: Sat 23 Sep 2017 08:27:21 PM CST
 ************************************************************************/

#ifndef _WLOG_H_
#define _WLOG_H_

#include <stdint.h>
#include <stddef.h>

#define WLOG_RET_ERR                -1
#define WLOG_RET_SUCESS             0

typedef enum
{
    WLOG_LERVEL_DEBUG = 0x01,
    WLOG_LERVEL_INFO = 0x02,
    WLOG_LERVEL_NOTICE = 0x04,
    WLOG_LERVEL_WARN = 0x08,
    WLOG_LERVEL_ERROR = 0x10,
    WLOG_LERVEL_FATAL = 0x20,
}wlog_level_t;

typedef uint32_t (*get_timestem_func_t)(void);

int wlog_init(const char *cfg_file, get_timestem_func_t timestem_func);

void *wlog_get_handle(const char *name , int thr_num);

int wlog(void *handle, int thr_id, const char *file, size_t filelen, const char *func, size_t funclen, 
        long line, wlog_level_t level, const char *format, ...);


#define wlog_debug(_han, _tid, format, args...) wlog(_han, _tid, __FILE__, sizeof(__FILE__) - 1, \
        __func__, sizeof(__func__) - 1, __LINE__, WLOG_LERVEL_DEBUG, format, ##args)
#define wlog_info(_han, _tid, format, args...) wlog(_han, _tid, __FILE__, sizeof(__FILE__) - 1, \
        __func__, sizeof(__func__) - 1, __LINE__, WLOG_LERVEL_INFO, format, ##args)
#define wlog_notice(_han, _tid, format, args...) wlog(_han, _tid, __FILE__, sizeof(__FILE__) - 1, \
        __func__, sizeof(__func__) - 1, __LINE__, WLOG_LERVEL_NOTICE, format, ##args)
#define wlog_warn(_han, _tid, format, args...) wlog(_han, _tid, __FILE__, sizeof(__FILE__) - 1, \
        __func__, sizeof(__func__) - 1, __LINE__, WLOG_LERVEL_WARN, format, ##args)
#define wlog_error(_han, _tid, format, args...) wlog(_han, _tid, __FILE__, sizeof(__FILE__) - 1, \
        __func__, sizeof(__func__) - 1, __LINE__, WLOG_LERVEL_ERROR, format, ##args)
#define wlog_fatal(_han, _tid, format, args...) wlog(_han, _tid, __FILE__, sizeof(__FILE__) - 1, \
        __func__, sizeof(__func__) - 1, __LINE__, WLOG_LERVEL_FATAL, format, ##args)

#endif
