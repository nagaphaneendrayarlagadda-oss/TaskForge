#include "monitor.h"

#include <stdio.h>

int monitor_init(
    monitor_t *monitor)
{
    if (monitor == NULL)
        return -1;

    monitor->count = 0;

    if (pthread_mutex_init(&monitor->mutex, NULL) != 0)
        return -1;

    return 0;
}

int monitor_add(
    monitor_t *monitor,
    const task_t *task)
{
    if (monitor == NULL || task == NULL)
        return -1;

    pthread_mutex_lock(&monitor->mutex);

    if (monitor->count >= MAX_TASKS)
    {
        pthread_mutex_unlock(&monitor->mutex);
        return -1;
    }

    monitor->records[monitor->count].task = *task;
    monitor->records[monitor->count].status = TASK_QUEUED;

    monitor->count++;

    pthread_mutex_unlock(&monitor->mutex);

    return 0;
}

int monitor_set_status(
    monitor_t *monitor,
    int task_id,
    task_status_t status)
{
    if (monitor == NULL)
        return -1;

    pthread_mutex_lock(&monitor->mutex);

    for (size_t i = 0; i < monitor->count; i++)
    {
        if (monitor->records[i].task.id == task_id)
        {
            monitor->records[i].status = status;

            pthread_mutex_unlock(&monitor->mutex);

            return 0;
        }
    }

    pthread_mutex_unlock(&monitor->mutex);

    return -1;
}

const char *monitor_status_string(
    task_status_t status)
{
    switch (status)
    {
        case TASK_QUEUED:
            return "QUEUED";

        case TASK_RUNNING:
            return "RUNNING";

        case TASK_COMPLETED:
            return "COMPLETED";

        case TASK_FAILED:
            return "FAILED";

        default:
            return "UNKNOWN";
    }
}

void monitor_print(
    monitor_t *monitor)
{
    if (monitor == NULL)
        return;

    pthread_mutex_lock(&monitor->mutex);

    printf("\n");
    printf("==============================================\n");
    printf("              TASKFORGE STATUS\n");
    printf("==============================================\n");

    printf(
        "%-5s %-10s %-10s %-20s\n",
        "ID",
        "PRIORITY",
        "DURATION",
        "NAME"
    );

    printf("----------------------------------------------\n");

    for (size_t i = 0; i < monitor->count; i++)
    {
        printf(
            "%-5d %-10d %-10d %-20s\n",
            monitor->records[i].task.id,
            monitor->records[i].task.priority,
            monitor->records[i].task.duration_ms,
            monitor_status_string(
                monitor->records[i].status
            )
        );
    }

    printf("==============================================\n");

    pthread_mutex_unlock(&monitor->mutex);

    fflush(stdout);
}

void monitor_destroy(
    monitor_t *monitor)
{
    if (monitor == NULL)
        return;

    pthread_mutex_destroy(&monitor->mutex);
}
