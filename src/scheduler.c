#include "scheduler.h"

#include <stdio.h>
#include <string.h>

int scheduler_init(scheduler_t *scheduler)
{
    if (scheduler == NULL)
        return -1;

    scheduler->size = 0;
    scheduler->stopping = 0;

    if (pthread_mutex_init(&scheduler->mutex, NULL) != 0)
        return -1;

    if (pthread_cond_init(&scheduler->not_empty, NULL) != 0)
    {
        pthread_mutex_destroy(&scheduler->mutex);
        return -1;
    }

    return 0;
}

int scheduler_submit(
    scheduler_t *scheduler,
    const task_t *task)
{
    if (scheduler == NULL || task == NULL)
        return -1;

    pthread_mutex_lock(&scheduler->mutex);

    if (scheduler->stopping || scheduler->size >= QUEUE_CAPACITY)
    {
        pthread_mutex_unlock(&scheduler->mutex);
        return -1;
    }

    scheduler->items[scheduler->size] = *task;
    scheduler->size++;

    /*
     * Higher priority value = higher priority.
     * Example:
     * 10 > 8 > 5 > 2
     */
    for (size_t i = scheduler->size - 1; i > 0; i--)
    {
        if (scheduler->items[i].priority >
            scheduler->items[i - 1].priority)
        {
            task_t temp = scheduler->items[i];

            scheduler->items[i] =
                scheduler->items[i - 1];

            scheduler->items[i - 1] = temp;
        }
        else
        {
            break;
        }
    }

    pthread_cond_signal(&scheduler->not_empty);

    pthread_mutex_unlock(&scheduler->mutex);

    return 0;
}

int scheduler_get_next(
    scheduler_t *scheduler,
    task_t *task)
{
    if (scheduler == NULL || task == NULL)
        return -1;

    pthread_mutex_lock(&scheduler->mutex);

    while (scheduler->size == 0 && !scheduler->stopping)
    {
        pthread_cond_wait(
            &scheduler->not_empty,
            &scheduler->mutex
        );
    }

    if (scheduler->size == 0 && scheduler->stopping)
    {
        pthread_mutex_unlock(&scheduler->mutex);
        return -1;
    }

    *task = scheduler->items[0];

    for (size_t i = 1; i < scheduler->size; i++)
    {
        scheduler->items[i - 1] =
            scheduler->items[i];
    }

    scheduler->size--;

    pthread_mutex_unlock(&scheduler->mutex);

    return 0;
}

void scheduler_stop(scheduler_t *scheduler)
{
    if (scheduler == NULL)
        return;

    pthread_mutex_lock(&scheduler->mutex);

    scheduler->stopping = 1;

    pthread_cond_broadcast(&scheduler->not_empty);

    pthread_mutex_unlock(&scheduler->mutex);
}

size_t scheduler_size(
    scheduler_t *scheduler)
{
    if (scheduler == NULL)
        return 0;

    pthread_mutex_lock(&scheduler->mutex);

    size_t size = scheduler->size;

    pthread_mutex_unlock(&scheduler->mutex);

    return size;
}

void scheduler_destroy(
    scheduler_t *scheduler)
{
    if (scheduler == NULL)
        return;

    pthread_cond_destroy(&scheduler->not_empty);
    pthread_mutex_destroy(&scheduler->mutex);
}

