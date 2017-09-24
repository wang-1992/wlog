/*************************************************************************
    > File Name: main.c
    > Author: wangkunyao
    > Mail: wangkyao1992@163.com 
    > Created Time: Sat 23 Sep 2017 08:43:10 PM CST
 ************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/time.h>

#include "wlog.h"

volatile uint64_t jiffies;
volatile uint32_t jiffie;

static void *update_jiffie(void *arg)
{
    struct timeval tv;
    while (1)
    {
        gettimeofday(&tv, NULL);
        jiffie = tv.tv_sec;
        jiffies = tv.tv_usec / 1000 + tv.tv_sec * 1000;
        usleep(20);
    }
    return arg;
}

static void init_jiffies_thread()
{
    int ret;
    pthread_t thread_id;

    ret = pthread_create(&thread_id, NULL, update_jiffie, NULL);
    if (ret != 0)
    {
        exit(1);
    }
}

static uint32_t get_jiffie()
{
    return jiffie;
}

static void *handle;

static void *test_log_thr(void *arg)
{
    int thr_id = *((int *)arg);

    //int num = 2000000;
    int num = 100;
    int i;

    uint32_t start = time(NULL);

    for (i = 0; i < num; i++)
    {
        wlog(handle, thr_id, "thr_id:%d----------test %d-------------%s:%d\n", thr_id, i, __FILE__, __LINE__);
    }
    uint32_t end = time(NULL);

    printf("start:%u, end:%u, inter:%d\n", start, end, end-start);

    return arg;
}

int main()
{
    int thr_num = 2;
    int i;
    int thr_id[64];
    pthread_t tid[64];

    init_jiffies_thread();
    wlog_init(get_jiffie);
    handle = wlog_get_handle("./log/test/", "hello", 60, thr_num, NULL);

    sleep(2);

    for (i = 0; i < thr_num; i++)
    {
        thr_id[i] = i;
        pthread_create(&tid[i], NULL, test_log_thr, &(thr_id[i]));
    }

    for (i = 0; i < thr_num; i++)
    {
        pthread_join(tid[i], NULL);
    }

    return 0;
}
