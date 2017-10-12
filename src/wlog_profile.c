/*************************************************************************
    > File Name: wlog_profile.c
    > Author: wangkunyao
    > Mail: wangkyao1992@163.com 
    > Created Time: Wed 11 Oct 2017 06:39:10 AM CST
 ************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdarg.h>

#include "wlog_profile.h"

int wlog_profile_inner(wlog_profile_flag_t flag, const char *file, 
        const long line, const char *fmt, ...)
{
    va_list args;

    switch (flag)
    {
        case INNER_DEBUG:
            fprintf(stdout, "[DEBUG] (%s:%ld) ", file, line);
            break;
        case INNER_WARN:
            fprintf(stdout, "[WARN] (%s:%ld) ", file, line);
            break;
        case INNER_ERROR:
            fprintf(stdout, "[ERROR] (%s:%ld) ", file, line);
            break;
        default:
            return -1;
    }
    va_start(args, fmt);
    vfprintf(stdout, fmt, args);
    va_end(args);
    fflush(stdout);
    return 0;
}

