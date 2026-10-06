#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include "scheduler.h"
#include "monitor.h"

#include <pthread.h>
#include <stddef.h>

typedef struct
{
    pthread_t *workers;
    size_t worker_count;

    scheduler_t *scheduler;
    monitor_t *monitor;

    int running;

} thread_pool_t;

int thread_pool_init(
    thread_pool_t *pool,
    size_t worker_count,
    scheduler_t *scheduler,
    monitor_t *monitor
);

int thread_pool_start(
    thread_pool_t *pool
);

void thread_pool_stop(
    thread_pool_t *pool
);

void thread_pool_destroy(
    thread_pool_t *pool
);

#endif
