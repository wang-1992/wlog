/*************************************************************************
    > File Name: wlog.h
    > Author: wangkunyao
    > Mail: wangkyao1992@163.com 
    > Created Time: Sat 23 Sep 2017 08:27:21 PM CST
 ************************************************************************/

#ifndef _WLOG_H_
#define _WLOG_H_

#include <stdint.h>

typedef uint32_t (*get_timestem_func_t)(void);

typedef enum
{
    //多线程一个文件
    wlog_mod_single_file = 1,
    //多线程多文件
    wlog_mod_multiple_file,
}wlog_mod_t;

int wlog_init(get_timestem_func_t timestem_func);
void *wlog_get_handle(const char *path ,const char *pre_file, uint32_t interval, int thr_num, wlog_mod_t mod);
int wlog(void *handle, int thr_id, const char *format, ...);

#endif
