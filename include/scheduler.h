#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "task.h"
#include <pthread.h>
#include <stddef.h>

#define QUEUE_CAPACITY 128

typedef struct
{
    task_t items[QUEUE_CAPACITY];
    size_t size;

    pthread_mutex_t mutex;
    pthread_cond_t not_empty;

    int stopping;

} scheduler_t;

int scheduler_init(scheduler_t *scheduler);

int scheduler_submit(
    scheduler_t *scheduler,
    const task_t *task
);

int scheduler_get_next(
    scheduler_t *scheduler,
    task_t *task
);

void scheduler_stop(scheduler_t *scheduler);

size_t scheduler_size(
    scheduler_t *scheduler
);

void scheduler_destroy(
    scheduler_t *scheduler
);

#endif
