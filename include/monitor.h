#ifndef MONITOR_H
#define MONITOR_H

#include "task.h"
#include <pthread.h>
#include <stddef.h>

typedef enum
{
    TASK_QUEUED,
    TASK_RUNNING,
    TASK_COMPLETED,
    TASK_FAILED
} task_status_t;

typedef struct
{
    task_t task;
    task_status_t status;
} task_record_t;

#define MAX_TASKS 128

typedef struct
{
    task_record_t records[MAX_TASKS];
    size_t count;

    pthread_mutex_t mutex;

} monitor_t;

int monitor_init(
    monitor_t *monitor
);

int monitor_add(
    monitor_t *monitor,
    const task_t *task
);

int monitor_set_status(
    monitor_t *monitor,
    int task_id,
    task_status_t status
);

void monitor_print(
    monitor_t *monitor
);

const char *monitor_status_string(
    task_status_t status
);

void monitor_destroy(
    monitor_t *monitor
);

#endif
