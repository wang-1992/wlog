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
#define NONE                 "\e[0m"
#define BLACK                "\e[0;30m"
#define L_BLACK              "\e[1;30m"
#define RED                  "\e[0;31m"
#define L_RED                "\e[1;31m"
#define GREEN                "\e[0;32m"
#define L_GREEN              "\e[1;32m"
#define BROWN                "\e[0;33m"
#define YELLOW               "\e[1;33m"
#define BLUE                 "\e[0;34m"
#define L_BLUE               "\e[1;34m"
#define PURPLE               "\e[0;35m"
#define L_PURPLE             "\e[1;35m"
#define CYAN                 "\e[0;36m"
#define L_CYAN               "\e[1;36m"
#define GRAY                 "\e[0;37m"
#define WHITE                "\e[1;37m"

#define BOLD                 "\e[1m"
#define UNDERLINE            "\e[4m"
#define BLINK                "\e[5m"
#define REVERSE              "\e[7m"
#define HIDE                 "\e[8m"
#define CLEAR                "\e[2J"
#define CLRLINE              "\r\e[K"

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
            fprintf(stdout, "[\033[1;47;34mWARN\033[0m] (%s:%ld) ", file, line);
            break;
        case INNER_ERROR:
            fprintf(stdout, "[\033[1;41;33mERROR\033[0m] (%s:%ld) ", file, line);
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

