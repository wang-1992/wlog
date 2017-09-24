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
    return NULL;
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
    printf("%u\n", jiffie);
    return jiffie;
}


int main()
{
    init_jiffies_thread();
    wlog_init(get_jiffie);
    void *handle = wlog_get_handle("./log/test/", "hello", 300, 1, NULL);
    sleep(1);
    while (1)
    {
        wlog(handle, 0, "test----------[%s:%d]\n", __FILE__, __LINE__);
        while(1)
        {
            sleep(1);
        }
    }
    return 0;
}
