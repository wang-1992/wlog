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

#define WLOG_GET_TIME_SLOT(_h, _ts)  \
    do{\
        uint32_t _ct;   \
        WLOG_GET_TIME(_ct);    \
        switch (_h->interval)   \
        {\
            case ONE_DEY_SECONDS:   \
                _ts = _ct / ONE_DEY_SECONDS * ONE_DEY_SECONDS - (8 * ONE_HOUR_SECONDS);  \
                break;\
            default:    \
                _ts = _ct / (_h->interval) * (_h->interval);    \
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
    unsigned char path_name[MAX_PATH_NAME + 1];
    unsigned char pre_file_name[MAX_PRE_NAME_SIZE + 1];
    uint32_t interval;
    uint32_t current_time_slot;
    int thr_num;
    char *buf[WLOG_MAX_THR_NUM];

    int (*wlog_write_func)(struct wlog_handle_t_ *, int, va_list, const char *);

    //每个线程使用一个句柄时使用
    int thr_fd[WLOG_MAX_THR_NUM];

    int new_fd;
    int old_fd;
}wlog_handle_t;


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
    struct tm *ptm;

    ptm = localtime(&time_slot);

    snprintf(file_name, MAX_FILE_NAME, "%s/%04d%02d%02d/%s_%04d%02d%02d%02d%02d", handle->path_name, 
            ptm->tm_year + 1900,ptm->tm_mon + 1,ptm->tm_mday,
            handle->pre_file_name,ptm->tm_year + 1900, 
            ptm->tm_mon + 1, ptm->tm_mday,ptm->tm_hour, ptm->tm_min);

    wlog_mkdirs_with_file(file_name);

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

static int wlog_swap_file_single(wlog_handle_t *handle)
{
    char file_name[MAX_FILE_NAME] = {0};

    wlog_make_file_name(handle, file_name);

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
            if (handle_g[i]->new_fd <= 0)
            {
                WLOG_GET_TIME_SLOT(handle_g[i], handle_g[i]->current_time_slot);
                wlog_swap_file_single(handle_g[i]);
            }
            else
            {
                //需要判断间隔，是否将日志写到新的文件中
                ret = wlog_is_create_new(handle_g[i]);
                if (ret)
                {
                    WLOG_GET_TIME_SLOT(handle_g[i], handle_g[i]->current_time_slot);
                    wlog_swap_file_single(handle_g[i]);
                }
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


int wlog_init(get_timestem_func_t timestem_func)
{
    pthread_t tid;
    int ret;

    if (wlog_init_flag)
    {
        fprintf(stderr, "wlog alread inited [%s:%d]\n", __FILE__, __LINE__);
        return -1;
    }

    ret = pthread_create(&tid, NULL, wlog_swap_file_thr, NULL);
    if (ret)
    {
        exit(1);
    }

    get_timestem_func = timestem_func;

    wlog_init_flag = 1;

    return 0;
}

void *wlog_get_handle(const char *path ,const char *pre_file, uint32_t interval, int thr_num)
{
    wlog_handle_t *handle;
    int i;

    if (wlog_init_flag == 0)
    {
        fprintf(stderr, "wlog not init [%s:%d]\n", __FILE__, __LINE__);
        return NULL;
    }

    if (path == NULL || pre_file == NULL || wlog_handle_num >= WLOG_MAX_HANDLE_NUM)
    {
        fprintf(stderr, "wlog arg error [%s:%d]\n", __FILE__, __LINE__);
        return NULL;
    }

    if (thr_num > WLOG_MAX_THR_NUM)
    {
        fprintf(stderr, "wlog thr num too big [%s:%d]\n", __FILE__, __LINE__);
        return NULL;
    }

    handle = calloc(sizeof(wlog_handle_t), 1);
    handle->interval = interval;
    handle->thr_num = thr_num;
    strncpy((char *)handle->path_name, path, MAX_PATH_NAME);
    strncpy((char *)handle->pre_file_name, pre_file, MAX_PRE_NAME_SIZE);

    handle->wlog_write_func = wlog_single;

    for (i = 0; i < thr_num; i++)
    {
        handle->buf[i] = malloc(WLOG_MAX_LOG_BUF);
    }

    pthread_mutex_lock(&wlog_mutex);

    handle_g[wlog_handle_num] = handle;
    wlog_handle_num++;

    pthread_mutex_unlock(&wlog_mutex);

    return handle;
}

int wlog(void *handle, int thr_id, const char *file, size_t filelen, const char *func, size_t funclen, 
        long line, wlog_level_t level, const char *format, ...)
{
    wlog_handle_t *wlog_handle = handle;
    va_list vp;

    if (wlog_handle == NULL)
    {
        fprintf(stderr, "wlog handle is NULL [%s:%d]\n", __FILE__, __LINE__);
        return -1;
    }
    va_start(vp, format);

    wlog_handle->wlog_write_func(wlog_handle, thr_id, vp, format);

    va_end(vp);

    return 0;
}
