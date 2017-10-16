/*************************************************************************
    > File Name: wlog_cfg.h
    > Author: wangkunyao
    > Mail: wangkyao1992@163.com 
    > Created Time: Thu 12 Oct 2017 05:24:28 AM CST
 ************************************************************************/

#ifndef _WLOG_CFG_H_
#define _WLOG_CFG_H_

#include "wlog.h"

#define WLOG_MAX_LINE                       4096
#define WLOG_MAX_BUF                        4096

typedef enum
{
    wlog_fmt_type_time = 1,
    wlog_fmt_type_line,
    wlog_fmt_type_file,
    wlog_fmt_type_func,
    wlog_fmt_type_log,
}wlog_fmt_type_t;

typedef struct
{
    char fmt[WLOG_MAX_BUF + 1];
    wlog_fmt_type_t fmt_type;
}wlog_fmt_str_t;

typedef struct
{
    char name[WLOG_MAX_BUF + 1];
    char pattern[WLOG_MAX_BUF + 1];
#define MAX_WLOG_FMT_STR_NUM                    20
    wlog_fmt_str_t wlog_fmt_str[MAX_WLOG_FMT_STR_NUM];
    int wlog_fmt_str_num;
}wlog_formats_t;

typedef enum
{
    wlog_out_file_type_file = 1,
    wlog_out_file_type_stdout,
    wlog_out_file_type_stderr,
}wlog_out_file_type;

typedef struct
{
    size_t file_size;
    uint32_t interval;
}wlog_file_limit_t;

typedef struct
{
    //指向该rule对应的format
    wlog_formats_t *wlog_formats;
    wlog_level_t level;
    char category[WLOG_MAX_BUF + 1];
    //表明改rule下的日志输出方式，当输出到文件时pre_path file_name有效
    wlog_out_file_type out_file_type;
    char pre_path[WLOG_MAX_BUF + 1];
    char file_name[WLOG_MAX_BUF + 1];
    wlog_file_limit_t file_limit;
}wlog_rule_t;

typedef struct
{
#define MAX_WLOG_FMT_NUM                        20
    wlog_formats_t *wlog_formats[MAX_WLOG_FMT_NUM];
    int wlog_formats_num;
}wlog_cfg_t;

int wlog_parse_cfg(wlog_cfg_t *wlog_cfg, const char *file_name);

#endif
