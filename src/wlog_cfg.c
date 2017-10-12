/*************************************************************************
    > File Name: wlog_cfg.c
    > Author: wangkunyao
    > Mail: wangkyao1992@163.com 
    > Created Time: Mon 09 Oct 2017 11:10:28 PM CST
 ************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>

#include "wlog.h"
#include "wlog_profile.h"
#include "wlog_cfg.h"

#define WLOG_MAX_LINE                       4096
#define WLOG_MAX_BUF                        4096

#define WLOGSTRCASECMP(_s, _d)              (strncasecmp((_s), (_d), strlen(_d))==0)

typedef enum
{
    step_start = 0,
    step_formats = 1,
    step_rules,
}step_t;

typedef struct
{
    char name[WLOG_MAX_BUF + 1];
    char pattern[WLOG_MAX_BUF + 1];

    int a;
}wlog_formats_t;

static wlog_formats_t *wlog_formats_new(char *line)
{
    wlog_formats_t *wlog_formats = calloc(sizeof(wlog_formats_t), 1);
    int nread = 0;
    int ret = 0;
    const char *p_start;
    const char *p_end;

    ret = sscanf(line, " %[^= \t] = %n", wlog_formats->name, &nread);
    if (ret != 1)
    {
        wp_error("format[%s], syntax wrong\n", line);
        goto err;
    }
    if (*(line + nread) != '"')
    {
        wp_error("the 1st char of pattern is not \", line+nread[%s]\n", line + nread);
        goto err;
    }

    p_start = line + nread + 1;
    p_end = strrchr(p_start, '"');
    if (p_end - p_start > WLOG_MAX_BUF)
    {
        wp_error("pattern is too long ");
        goto err;
    }

    memcpy(wlog_formats->pattern, p_start, p_end - p_start);


    return wlog_formats;
err:
    free(wlog_formats);
    return NULL;
}

static int wlog_cfg_parse_line(wlog_cfg_t *wlog_cfg, char *line, step_t *step)
{
    char name[WLOG_MAX_LINE + 1];
    //便签行
    if (line[0] == '[')
    {
        step_t lstep = *step;

        sscanf(line, "[ %[^] \t ]", name);
        if (WLOGSTRCASECMP(name, "formats"))
        {
            *step = step_formats;
        }
        else if (WLOGSTRCASECMP(name, "rules"))
        {
            *step = step_rules;
        }

        if (lstep >= *step)
        {
            wp_error("must follow formats->rules\n");
            return -1;
        }
        
        return 0;
    }

    switch (*step)
    {
        case step_formats:
            wlog_formats_new(line);
            break;
        case step_rules:
            break;
        default:
            wp_error("not in any step\n");
            return -1;
    }

    return 0;
}


int wlog_parse_cfg(wlog_cfg_t *wlog_cfg, const char *file_name)
{
    struct stat file_stat;
    FILE *fp = NULL;
    char line[WLOG_MAX_LINE + 1];
    size_t line_len;
    char *p = NULL;
    char *pline = NULL;
    int i;
    int in_quotation = 0;
    int ret;
    
    step_t step = step_start;

    if (file_name == NULL)
    {
        wp_error("file is null\n");
        goto err;
    }

    if (lstat(file_name, &file_stat))
    {
        wp_error("lstat conf file[%s] fail, errno[%d]\n", file_name, errno);
        goto  err;
    }

    fp = fopen(file_name, "r");
    if (fp == NULL)
    {
        wp_error("open file[%s] fail, errno[%d]\n", file_name, errno);
        goto err;
    }

    memset(line, 0, sizeof(line));

    pline = line;
    while (fgets(pline, sizeof(line) - (pline - line), fp) != NULL)
    {
        line_len = strlen(pline);
        if (pline[line_len - 1] == '\n')
        {
            pline[line_len - 1] = '\0';
        }

        p = pline;

        while (*p && isspace((int)*p))
        {
            ++p;
        }

        if (*p == '\0' || *p == '#')
        {
            continue;
        }

        for (i = 0; p[i] != '\0'; i++)
        {
            pline[i] = p[i];
        }
        pline[i] = '\0';
        for (p = pline + strlen(pline) - 1; isspace((int)*p); --p)
            ;
        if (*p == '\\')
        {
            if ((p - line) > WLOG_MAX_LINE- 30)
            {
                pline = line;
            }
            else
            {
                for (p--; isspace((int)*p); --p)
                    ;
                p++;
                *p = 0;
                pline = p;
                continue;
            }
        }
        else
        {
            pline = line;
        }

        *++p = '\0';
        in_quotation = 0;
        for (p = line; *p != '\0'; p++)
        {
            if (*p == '"')
            {
                in_quotation ^= 1;
                continue;
            }
            if (*p == '#' && !in_quotation)
            {
                *p = '\0';
                break;
            }
        }
        //finish parse line
        ret = wlog_cfg_parse_line(wlog_cfg, line, &step);
        if (ret == -1)
        {
            goto err;
        }
    }

    fclose(fp);

    return WLOG_RET_SUCESS;
err:
    return WLOG_RET_ERR;
}

#ifdef _TEST_CFG_

int main()
{
    wlog_cfg_t wlog_cfg;
    wlog_parse_cfg(&wlog_cfg, "./wlog_cfg.conf");
    return 0;
}

#endif
