#define _POSIX_C_SOURCE 200809L

#include "thread_pool.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>

/*
 * Sleep for the exact number of milliseconds.
 */
static void sleep_for_milliseconds(int duration_ms)
{
    struct timespec request;

    request.tv_sec = duration_ms / 1000;

    request.tv_nsec =
        (long)(duration_ms % 1000) * 1000000L;

    /*
     * Continue sleeping if interrupted by a signal.
     */
    while (nanosleep(&request, &request) == -1)
    {
        if (errno != EINTR)
        {
            break;
        }
    }
}

/*
 * Worker thread function.
 */
static void *worker_function(void *arg)
{
    thread_pool_t *pool =
        (thread_pool_t *)arg;

    while (pool->running)
    {
        task_t task;

        /*
         * Get the highest-priority task
         * from the scheduler.
         */
        if (scheduler_get_next(
                pool->scheduler,
                &task) != 0)
        {
            break;
        }

        /*
         * Task is now running.
         */
        monitor_set_status(
            pool->monitor,
            task.id,
            TASK_RUNNING
        );

        printf(
            "[WORKER] job=%d priority=%d "
            "name=%s duration=%d ms\n",
            task.id,
            task.priority,
            task.name,
            task.duration_ms
        );

        fflush(stdout);

        /*
         * Execute the task for the
         * requested duration.
         */
        sleep_for_milliseconds(
            task.duration_ms
        );

        /*
         * Task completed successfully.
         */
        monitor_set_status(
            pool->monitor,
            task.id,
            TASK_COMPLETED
        );

        printf(
            "[WORKER] completed job=%d name=%s\n",
            task.id,
            task.name
        );

        fflush(stdout);
    }

    return NULL;
}

/*
 * Initialize the thread pool.
 */
int thread_pool_init(
    thread_pool_t *pool,
    size_t worker_count,
    scheduler_t *scheduler,
    monitor_t *monitor)
{
    if (pool == NULL ||
        scheduler == NULL ||
        monitor == NULL ||
        worker_count == 0)
    {
        return -1;
    }

    pool->workers = malloc(
        sizeof(pthread_t) * worker_count
    );

    if (pool->workers == NULL)
    {
        return -1;
    }

    pool->worker_count = worker_count;
    pool->scheduler = scheduler;
    pool->monitor = monitor;
    pool->running = 1;

    return 0;
}

/*
 * Start all worker threads.
 */
int thread_pool_start(thread_pool_t *pool)
{
    if (pool == NULL)
    {
        return -1;
    }

    for (size_t i = 0;
         i < pool->worker_count;
         i++)
    {
        if (pthread_create(
                &pool->workers[i],
                NULL,
                worker_function,
                pool) != 0)
        {
            pool->running = 0;

            scheduler_stop(
                pool->scheduler
            );

            for (size_t j = 0; j < i; j++)
            {
                pthread_join(
                    pool->workers[j],
                    NULL
                );
            }

            return -1;
        }
    }

    return 0;
}

/*
 * Stop the thread pool.
 */
void thread_pool_stop(thread_pool_t *pool)
{
    if (pool == NULL)
    {
        return;
    }

    pool->running = 0;

    scheduler_stop(
        pool->scheduler
    );

    /*
     * Wait for every worker thread
     * to finish.
     */
    for (size_t i = 0;
         i < pool->worker_count;
         i++)
    {
        pthread_join(
            pool->workers[i],
            NULL
        );
    }
}

/*
 * Destroy the thread pool.
 */
void thread_pool_destroy(thread_pool_t *pool)
{
    if (pool == NULL)
    {
        return;
    }

    free(pool->workers);

    pool->workers = NULL;
    pool->worker_count = 0;
    pool->scheduler = NULL;
    pool->monitor = NULL;
    pool->running = 0;
}
