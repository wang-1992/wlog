/*************************************************************************
    > File Name: wlog.c
    > Author: wangkunyao
    > Mail: wangkyao1992@163.com 
    > Created Time: Sat 23 Sep 2017 04:39:05 PM CST
 ************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>

#include "wlog.h"

#define MAX_PRE_NAME_SIZE               1024
#define MAX_PATH_NAME                   1024
#define MAX_FILE_NAME                   MAX_PRE_NAME_SIZE + MAX_PATH_NAME + 1
#define WLOG_MAX_THR_NUM                64

#define ONE_DEY_SECONDS         28800

#define WLOG_GET_TIME(_t)   \
    do{\
        if (get_timestem_func)\
        {   \
            _t = get_timestem_func();   \
        }else{\
            _t = time(NULL);    \
        }   \
    }while(0)

#define WLOG_GET_TIME_SLOT(_h, _ts)  \
    do{\
        uint32_t _ct;   \
        WLOG_GET_TIME(_ct);    \
        switch (_h->interval)   \
        {\
            case ONE_DEY_SECONDS:   \
                _ts = _ct / ONE_DEY_SECONDS * ONE_DEY_SECONDS - (8 * ONE_DEY_SECONDS);  \
                break;\
            default:    \
                _ts = _ct / (_h->interval) * (_h->interval);    \
                break;  \
        }\
    }while(0)


typedef struct
{
    unsigned char path_name[MAX_PATH_NAME + 1];
    unsigned char pre_file_name[MAX_PRE_NAME_SIZE + 1];
    uint32_t interval;
    uint32_t current_time_slot;
    FILE *fp[WLOG_MAX_THR_NUM];
    void *user;
    int thr_num;
    unsigned char current_file[MAX_FILE_NAME + 1];
}wlog_handle_t;


static get_timestem_func_t get_timestem_func;

static int wlog_is_path_exists(char *path)
{
    int res;
    res=access(path,F_OK);
    return res==0?1:0;
}

static int wlog_is_file_exists(char *path)
{
    return wlog_is_path_exists(path);
}

static int wlog_mkdirs(char *dirs)
{
	char dir[MAX_PATH_NAME];
	int len,i,res;

	len = strlen(dirs);
	if (len > MAX_PATH_NAME - 2)//一个是为'\0'保留,一个是为'/'保留
	{
		//debug(MK_LV(FILE_M,DE),thr_id,"###dirs is too long(>127):%s\n");
		return 0;
	}
	strcpy(dir,dirs);
	if (dir[len-1] != '/')
	{
		strcat(dir,"/");
		len++;
	}

	i = 0;
	if (dir[i] == '/')
    {
		i++;
    }

    while (i < len)
    {
        if (dir[i] == '/')
        {
            dir[i] = 0;

            if (wlog_is_path_exists(dir) == 0)
            {
                res = mkdir(dir, 0775);
                if (res < 0)
                {
                    if(errno != EEXIST)
                    {
                        return 0;

                    }
                }
            }
            dir[i] = '/';
        }
        i++;
    }

	return 1;
}

static int wlog_mkdirs_with_file(char *path)
{
    int t;
	//生成路径
	//获取目录部分
	t = strlen(path);
	if (t <= 0)
	{
		return 0;
	}
	
	while (path[t] != '/')
	{
		t--;
		if (t == 0)
		{
			break;
		}
	}

	if (t > 0)
	{
		path[t]=0;
		if (wlog_mkdirs(path)==0)
		{
			path[t] = '/';
			return 0;
		}
		path[t] = '/';
	}

    return 0;
}




static int wlog_make_file_name(wlog_handle_t *handle, int thr_id)
{
    time_t time_slot = handle->current_time_slot;
    struct tm *ptm;

    ptm = localtime(&time_slot);

    snprintf((char *)handle->current_file, MAX_FILE_NAME, "%s/%04d/%02d%02d/%s_%02d_%02d_%d.log", handle->path_name, 
            ptm->tm_year + 1900, ptm->tm_mon + 1, ptm->tm_mday,
            handle->pre_file_name, ptm->tm_hour, ptm->tm_min, thr_id);

    wlog_mkdirs_with_file((char *)handle->current_file);

    return 0;
}

static int wlog_is_create_new(wlog_handle_t *handle)
{
    uint32_t current_time_slot;

    if (handle->interval == 0)
    {
        return 0;
    }
    else
    {
        WLOG_GET_TIME_SLOT(handle, current_time_slot);
        if (handle->current_time_slot < current_time_slot)
        {
            return 1;
        }
    }
    return 0;
}

static int wlog_swap_file(wlog_handle_t *handle, int thr_id)
{
    wlog_make_file_name(handle, thr_id);
    if (handle->fp[thr_id] != NULL)
    {
        fclose(handle->fp[thr_id]);
    }

    if (wlog_is_file_exists((char *)handle->current_file) == 0)
    {
        //文件不存在
        handle->fp[thr_id] = fopen((char *)handle->current_file, "wb+");
    }
    else
    {
        handle->fp[thr_id] = fopen((char *)handle->current_file, "ab+");
    }

    return 0;

}



int wlog_init(get_timestem_func_t timestem_func)
{
    get_timestem_func = timestem_func;
    return 0;
}

void *wlog_get_handle(const char *path ,const char *pre_file, uint32_t interval, int thr_num, void *user)
{
    wlog_handle_t *handle;
    if (path == NULL || pre_file == NULL)
    {
        return NULL;
    }
    handle = calloc(sizeof(wlog_handle_t), 1);
    handle->current_time_slot = 0;
    handle->interval = interval;
    handle->user = user;
    handle->thr_num = thr_num;
    strncpy((char *)handle->path_name, path, MAX_PATH_NAME);
    strncpy((char *)handle->pre_file_name, pre_file, MAX_PRE_NAME_SIZE);

    return handle;
}

int wlog(void *handle, int thr_id, const char *format, ...)
{
    wlog_handle_t *wlog_handle = handle;
    int ret;

    if (wlog_handle == NULL)
    {
        return -1;
    }

    //第一次调用，必须要生成文件
    if (wlog_handle->fp[thr_id] == NULL)
    {
        WLOG_GET_TIME_SLOT(wlog_handle, wlog_handle->current_time_slot);
        wlog_swap_file(wlog_handle, thr_id);
    }
    else
    {
        //需要判断间隔，是否将日志写到新的文件中
        ret = wlog_is_create_new(handle);
        if (ret)
        {
            WLOG_GET_TIME_SLOT(wlog_handle, wlog_handle->current_time_slot);
            wlog_swap_file(wlog_handle, thr_id);
        }
    }

    time_t cur_time;
    WLOG_GET_TIME(cur_time);
    struct tm *ptm;
    ptm = localtime(&cur_time);

    fprintf(wlog_handle->fp[thr_id], "[%04d-%02d-%02d %02d:%02d:%02d] ",
            ptm->tm_year + 1900, ptm->tm_mon + 1, ptm->tm_mday,
            ptm->tm_hour, ptm->tm_min, ptm->tm_sec);

    va_list vp;
    va_start(vp, format);
    vfprintf(wlog_handle->fp[thr_id], format, vp);
    va_end(vp);
    fflush(wlog_handle->fp[thr_id]);

    return 0;
}
