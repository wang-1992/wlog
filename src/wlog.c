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
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/limits.h>

#include "wlog.h"
#include "wlog_cfg.h"
#include "wlog_profile.h"

#define MAX_PRE_NAME_SIZE               1024
#define MAX_PATH_NAME                   1024
#define MAX_FILE_NAME                   MAX_PRE_NAME_SIZE + MAX_PATH_NAME + 1
#define WLOG_MAX_THR_NUM                64
#define WLOG_MAX_HANDLE_NUM             1024
#define WLOG_MAX_LOG_BUF                PIPE_BUF

#define ONE_HOUR_SECONDS		3600
#define ONE_DEY_SECONDS         (ONE_HOUR_SECONDS * 24)

#define WLOG_GET_TIME(_t)   \
    do{\
        if (get_timestem_func)\
        {   \
            _t = get_timestem_func();   \
        }else{\
            _t = time(NULL);    \
        }   \
    }while(0)

#define WLOG_GET_TIME_SLOT(_i, _ts)  \
    do{\
        uint32_t _ct;   \
        WLOG_GET_TIME(_ct);    \
        switch (_i)   \
        {\
            case ONE_DEY_SECONDS:   \
                _ts = _ct / ONE_DEY_SECONDS * ONE_DEY_SECONDS - (8 * ONE_HOUR_SECONDS);  \
                break;\
            case 0: \
                _ts = 0;    \
                break;  \
            default:    \
                _ts = _ct / (_i) * (_i);    \
                break;  \
        }\
    }while(0)

#define WLOG_FILE_APPEND            (O_WRONLY|O_APPEND)
#define WLOG_FILE_CREATE_OR_OPEN    O_CREAT
#define WLOG_FILE_WRONLY            O_WRONLY 
#define WLOG_FILE_DEFAULT_ACCESS    0644
#define WLOG_FILE_NONBLOCK          O_NONBLOCK
//#define WLOG_FILE_NONBLOCK          0

#define wlog_open_file(name, mode, create, nblock, access) \
    open((const char *) name, mode|create|nblock, access)



typedef struct wlog_handle_t_
{
    wlog_rule_t *wlog_rule;
    uint32_t current_time_slot;
    uint64_t current_file_size;

    int thr_num;
    char *buf[WLOG_MAX_THR_NUM];

    int new_fd;
    int old_fd;
}wlog_handle_t;

//存储配置信息的结构体，信息从配置文件中读取
static wlog_cfg_t wlog_cfg;

static get_timestem_func_t get_timestem_func;
static wlog_handle_t *handle_g[WLOG_MAX_HANDLE_NUM];
static int wlog_handle_num = 0;
static uint8_t wlog_init_flag;

static pthread_mutex_t wlog_mutex = PTHREAD_MUTEX_INITIALIZER;

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

static int wlog_make_file_name(wlog_handle_t *handle, char *file_name)
{
    time_t time_slot = handle->current_time_slot;
    wlog_rule_t *rule = handle->wlog_rule;
    struct tm *ptm;

    if (rule == NULL)
    {
        return -1;
    }

    ptm = localtime(&time_slot);

    snprintf(file_name, MAX_FILE_NAME, "%s/%04d%02d%02d/%s_%04d%02d%02d%02d%02d", rule->pre_path,
            ptm->tm_year + 1900,ptm->tm_mon + 1,ptm->tm_mday,
            rule->file_name, ptm->tm_year + 1900, 
            ptm->tm_mon + 1, ptm->tm_mday,ptm->tm_hour, ptm->tm_min);

    wlog_mkdirs_with_file(file_name);

    return 0;
}


static int wlog_swap_file(wlog_handle_t *handle)
{
    char file_name[MAX_FILE_NAME] = {0};
    int ret;

    ret = wlog_make_file_name(handle, file_name);
    if (ret == -1)
    {
        return -1;
    }

    if (handle->old_fd > 0)
    {
        close(handle->old_fd);
    }

    handle->old_fd = handle->new_fd;

    if (wlog_is_file_exists(file_name) == 0)
    {
        //文件不存在
        handle->new_fd = wlog_open_file(file_name, WLOG_FILE_APPEND, WLOG_FILE_CREATE_OR_OPEN, WLOG_FILE_NONBLOCK, WLOG_FILE_DEFAULT_ACCESS);
    }
    else
    {
        handle->new_fd = wlog_open_file(file_name, WLOG_FILE_APPEND, 0, WLOG_FILE_NONBLOCK, WLOG_FILE_DEFAULT_ACCESS);
    }

    return 0;

}

static int wlog_is_create_new(wlog_handle_t *wlog_handle)
{
    uint32_t current_time_slot;



    switch (wlog_handle->wlog_rule->out_file_type)
    {
    case wlog_out_file_type_stderr:
        wlog_handle->new_fd = STDERR_FILENO;
        //对于标准输入和输出不需要创建文件
        return 0;
    case wlog_out_file_type_stdout:
        wlog_handle->new_fd = STDOUT_FILENO;
        //对于标准输入和输出不需要创建文件
        return 0;
        break;
    default:
        break;
    }

    WLOG_GET_TIME_SLOT(wlog_handle->wlog_rule->file_limit.interval, current_time_slot);
    //第一次调用必须生成文件
    if (wlog_handle->new_fd <= 0)
    {
            
        goto new;
    }
    if (wlog_handle->wlog_rule->file_limit.interval != 0 && wlog_handle->current_time_slot < current_time_slot)
    {
        goto new;
    }
    if (wlog_handle->wlog_rule->file_limit.file_size != 0 
            && wlog_handle->wlog_rule->file_limit.file_size < wlog_handle->current_file_size)
    {
        goto new;
    }
    return 0;
new:
    wlog_handle->current_time_slot = current_time_slot;
    wlog_handle->current_file_size = 0;
    return 1;
}

static void *wlog_swap_file_thr(void *arg)
{
    int i;
    int ret;

    while (1)
    {
        usleep(100);
        pthread_mutex_lock(&wlog_mutex);
        for (i = 0; i < wlog_handle_num; i++)
        {
            //第一次调用，必须要生成文件
            if (handle_g[i]->wlog_rule == NULL)
            {
                continue;
            }
            ret = wlog_is_create_new(handle_g[i]);
            if (ret)
            {
                wlog_swap_file(handle_g[i]);
            }
        }
        pthread_mutex_unlock(&wlog_mutex);
    }
    return arg;
}

static int wlog_write(int fd, char *tmp_buf, va_list vp, const char *format)
{
    int len = 0;
    int ret;

    time_t cur_time;
    struct tm *ptm;

    WLOG_GET_TIME(cur_time);
    ptm = localtime(&cur_time);

    len = snprintf(tmp_buf, WLOG_MAX_LOG_BUF, "[%04d-%02d-%02d %02d:%02d:%02d] ",
            ptm->tm_year + 1900, ptm->tm_mon + 1, ptm->tm_mday,
            ptm->tm_hour, ptm->tm_min, ptm->tm_sec);

    len += vsnprintf(tmp_buf + len, WLOG_MAX_LOG_BUF - len, format, vp);

    if (fd > 0)
    {
        ret = write(fd, tmp_buf, len);
        if (ret < 0)
        {
            perror("write");
        }
    }

    return 0;
}

static int wlog_single(wlog_handle_t *handle, int thr_id, va_list vp, const char *format)
{
    wlog_write(handle->new_fd, handle->buf[thr_id], vp, format);

    return 0;
}

int wlog_init(const char *cfg_file, get_timestem_func_t timestem_func)
{
    pthread_t tid;
    int ret;

    if (wlog_init_flag)
    {
        wp_error("wlog alread inited\n");
        return -1;
    }

    wlog_parse_cfg(&wlog_cfg, cfg_file);

    ret = pthread_create(&tid, NULL, wlog_swap_file_thr, NULL);
    if (ret)
    {
        exit(1);
    }

    get_timestem_func = timestem_func;

    wlog_init_flag = 1;

    return 0;
}

void *wlog_get_handle(const char *name , int thr_num)
{
    wlog_handle_t *handle;
    int i;

    if (wlog_init_flag == 0)
    {
        wp_error("wlog not init\n");
        return NULL;
    }

    if (name == NULL)
    {
        return NULL;
    }

    if (wlog_handle_num >= WLOG_MAX_HANDLE_NUM)
    {
        return NULL;
    }

    handle = calloc(sizeof(wlog_handle_t), 1);

    handle->thr_num = thr_num;

    for (i = 0; i < thr_num; i++)
    {
        handle->buf[i] = malloc(WLOG_MAX_LOG_BUF);
    }

    for (i = 0; i < wlog_cfg.wlog_rule_num; i++)
    {
        if (WLOGSTRCASECMP(wlog_cfg.wlog_rule[i]->category, name))
        {
            handle->wlog_rule = wlog_cfg.wlog_rule[i];
        }
    }

    handle_g[wlog_handle_num] = handle;
    wlog_handle_num++;
    return handle;
}


int wlog(void *handle, int thr_id, const char *file, size_t filelen, const char *func, size_t funclen, 
        long line, wlog_level_t level, const char *format, ...)
{
    wlog_handle_t *wlog_handle = handle;
    va_list vp;

    if (wlog_handle == NULL)
    {
        wp_error("wlog handle is NULL\n");
        return -1;
    }
    va_start(vp, format);

    wlog_single(wlog_handle, thr_id, vp, format);

    va_end(vp);

    //printf("file = %.*s, func = %.*s, line = %ld, level= %d\n", (int)filelen,file,(int)funclen,func,line,level);

    return 0;
}
