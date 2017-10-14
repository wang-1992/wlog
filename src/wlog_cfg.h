/*************************************************************************
    > File Name: wlog_cfg.h
    > Author: wangkunyao
    > Mail: wangkyao1992@163.com 
    > Created Time: Thu 12 Oct 2017 05:24:28 AM CST
 ************************************************************************/

#ifndef _WLOG_CFG_H_
#define _WLOG_CFG_H_

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

typedef struct
{
    wlog_formats_t *wlog_formats;
}wlog_rule_t;

typedef struct
{
#define MAX_WLOG_FMT_NUM                        20
    wlog_formats_t *wlog_formats[MAX_WLOG_FMT_NUM];
    int wlog_formats_num;
}wlog_cfg_t;

#endif
