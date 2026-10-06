#define _POSIX_C_SOURCE 200809L

#include "ipc.h"
#include "scheduler.h"
#include "thread_pool.h"
#include "monitor.h"

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

static volatile sig_atomic_t server_running = 1;

/*
 * Handle Ctrl+C and termination signals.
 */
static void handle_signal(int signal_number)
{
    (void)signal_number;

    server_running = 0;
}

/*
 * Send a response to a client through
 * the client's private response FIFO.
 */
static void send_response(
    pid_t client_pid,
    ipc_response_t *response)
{
    char response_path[128];

    ipc_response_fifo_path(
        client_pid,
        response_path,
        sizeof(response_path)
    );

    int fd = open(
        response_path,
        O_WRONLY
    );

    if (fd == -1)
    {
        perror("[IPC] open response FIFO");
        return;
    }

    ssize_t bytes_written = write(
        fd,
        response,
        sizeof(ipc_response_t)
    );

    if (bytes_written !=
        (ssize_t)sizeof(ipc_response_t))
    {
        perror("[IPC] write response");
    }

    close(fd);
}

/*
 * Send current task status to a client.
 */
static void send_status_response(
    pid_t client_pid,
    monitor_t *monitor)
{
    ipc_response_t response;

    memset(
        &response,
        0,
        sizeof(response)
    );

    response.success = 1;
    response.task_id = 0;

    int offset = 0;

    offset += snprintf(
        response.text + offset,
        IPC_RESPONSE_TEXT - offset,
        "\n==============================================\n"
    );

    offset += snprintf(
        response.text + offset,
        IPC_RESPONSE_TEXT - offset,
        "              TASKFORGE STATUS\n"
    );

    offset += snprintf(
        response.text + offset,
        IPC_RESPONSE_TEXT - offset,
        "==============================================\n"
    );

    offset += snprintf(
        response.text + offset,
        IPC_RESPONSE_TEXT - offset,
        "%-5s %-10s %-10s %-20s %-12s\n",
        "ID",
        "PRIORITY",
        "DURATION",
        "NAME",
        "STATUS"
    );

    offset += snprintf(
        response.text + offset,
        IPC_RESPONSE_TEXT - offset,
        "------------------------------------------------------------\n"
    );

    pthread_mutex_lock(
        &monitor->mutex
    );

    for (size_t i = 0;
         i < monitor->count;
         i++)
    {
        offset += snprintf(
            response.text + offset,
            IPC_RESPONSE_TEXT - offset,
            "%-5d %-10d %-10d %-20s %-12s\n",
            monitor->records[i].task.id,
            monitor->records[i].task.priority,
            monitor->records[i].task.duration_ms,
            monitor->records[i].task.name,
            monitor_status_string(
                monitor->records[i].status
            )
        );

        if (offset >= IPC_RESPONSE_TEXT - 100)
        {
            break;
        }
    }

    pthread_mutex_unlock(
        &monitor->mutex
    );

    snprintf(
        response.text + offset,
        IPC_RESPONSE_TEXT - offset,
        "==============================================\n"
    );

    send_response(
        client_pid,
        &response
    );
}

/*
 * Send the result of a task submission.
 */
static void send_submit_response(
    pid_t client_pid,
    int success,
    int task_id,
    const char *message_text)
{
    ipc_response_t response;

    memset(
        &response,
        0,
        sizeof(response)
    );

    response.success = success;
    response.task_id = task_id;

    if (message_text != NULL)
    {
        snprintf(
            response.text,
            IPC_RESPONSE_TEXT,
            "%s",
            message_text
        );
    }

    send_response(
        client_pid,
        &response
    );
}

int main(void)
{
    scheduler_t scheduler;
    thread_pool_t pool;
    monitor_t monitor;

    int fifo_fd = -1;

    int next_task_id = 1;

    printf(
        "====================================\n"
    );

    printf(
        "        TASKFORGE SERVER\n"
    );

    printf(
        "====================================\n"
    );

    /*
     * Install signal handlers.
     */
    signal(
        SIGINT,
        handle_signal
    );

    signal(
        SIGTERM,
        handle_signal
    );

    /*
     * Initialize scheduler.
     */
    if (scheduler_init(&scheduler) != 0)
    {
        fprintf(
            stderr,
            "Failed to initialize scheduler\n"
        );

        return 1;
    }

    /*
     * Initialize monitor.
     */
    if (monitor_init(&monitor) != 0)
    {
        fprintf(
            stderr,
            "Failed to initialize monitor\n"
        );

        scheduler_destroy(
            &scheduler
        );

        return 1;
    }

    /*
     * Initialize thread pool.
     */
    if (thread_pool_init(
            &pool,
            4,
            &scheduler,
            &monitor) != 0)
    {
        fprintf(
            stderr,
            "Failed to initialize thread pool\n"
        );

        monitor_destroy(
            &monitor
        );

        scheduler_destroy(
            &scheduler
        );

        return 1;
    }

    /*
     * Start worker threads.
     */
    if (thread_pool_start(&pool) != 0)
    {
        fprintf(
            stderr,
            "Failed to start thread pool\n"
        );

        thread_pool_destroy(
            &pool
        );

        monitor_destroy(
            &monitor
        );

        scheduler_destroy(
            &scheduler
        );

        return 1;
    }

    /*
     * Create IPC FIFO.
     */
    if (ipc_server_create() != 0)
    {
        fprintf(
            stderr,
            "Failed to create IPC FIFO\n"
        );

        thread_pool_stop(
            &pool
        );

        thread_pool_destroy(
            &pool
        );

        monitor_destroy(
            &monitor
        );

        scheduler_destroy(
            &scheduler
        );

        return 1;
    }

    /*
     * Open IPC FIFO.
     */
    fifo_fd = ipc_server_open();

    if (fifo_fd < 0)
    {
        fprintf(
            stderr,
            "Failed to open IPC FIFO\n"
        );

        ipc_remove_fifo();

        thread_pool_stop(
            &pool
        );

        thread_pool_destroy(
            &pool
        );

        monitor_destroy(
            &monitor
        );

        scheduler_destroy(
            &scheduler
        );

        return 1;
    }

    printf(
        "[SERVER] TaskForge started\n"
    );

    printf(
        "[SERVER] Workers: 4\n"
    );

    printf(
        "[SERVER] IPC FIFO: %s\n",
        TASKFORGE_FIFO
    );

    printf(
        "[SERVER] Monitoring enabled\n"
    );

    printf(
        "[SERVER] Waiting for commands...\n"
    );

    fflush(stdout);

    /*
     * Main server loop.
     */
    while (server_running)
    {
        ipc_message_t message;

        memset(
            &message,
            0,
            sizeof(message)
        );

        int result = ipc_server_read(
            fifo_fd,
            &message
        );

        if (result < 0)
        {
            if (!server_running)
            {
                break;
            }

            continue;
        }

        if (result == 0)
        {
            continue;
        }

        /*
         * SUBMIT command.
         */
        if (message.command ==
            IPC_CMD_SUBMIT)
        {
            /*
             * Assign a unique Job ID.
             */
            message.task.id =
                next_task_id++;

            /*
             * Add task to scheduler.
             */
            if (scheduler_submit(
                    &scheduler,
                    &message.task) == 0)
            {
                /*
                 * Add task to monitor.
                 */
                if (monitor_add(
                        &monitor,
                        &message.task) != 0)
                {
                    fprintf(
                        stderr,
                        "[MONITOR] failed to add job=%d\n",
                        message.task.id
                    );
                }

                printf(
                    "[IPC] accepted job=%d "
                    "priority=%d "
                    "name=%s "
                    "duration=%d ms\n",
                    message.task.id,
                    message.task.priority,
                    message.task.name,
                    message.task.duration_ms
                );

                fflush(stdout);

                /*
                 * Send assigned Job ID
                 * back to taskctl.
                 */
                send_submit_response(
                    message.client_pid,
                    1,
                    message.task.id,
                    "Task accepted by TaskForge server.\n"
                );
            }
            else
            {
                fprintf(
                    stderr,
                    "[IPC] failed to submit job=%d\n",
                    message.task.id
                );

                /*
                 * Tell client that submission failed.
                 */
                send_submit_response(
                    message.client_pid,
                    0,
                    message.task.id,
                    "Task submission failed.\n"
                );
            }
        }

        /*
         * STATUS command.
         */
        else if (message.command ==
                 IPC_CMD_STATUS)
        {
            printf(
                "[IPC] Status request "
                "from client PID %d\n",
                (int)message.client_pid
            );

            fflush(stdout);

            send_status_response(
                message.client_pid,
                &monitor
            );
        }

        /*
         * SHUTDOWN command.
         */
        else if (message.command ==
                 IPC_CMD_SHUTDOWN)
        {
            printf(
                "[SERVER] Shutdown request received\n"
            );

            fflush(stdout);

            server_running = 0;
        }

        /*
         * Unknown command.
         */
        else
        {
            fprintf(
                stderr,
                "[IPC] Unknown command: %d\n",
                message.command
            );
        }
    }

    printf(
        "[SERVER] Shutting down...\n"
    );

    /*
     * Stop workers.
     */
    thread_pool_stop(
        &pool
    );

    /*
     * Close and remove IPC FIFO.
     */
    ipc_server_close(
        fifo_fd
    );

    ipc_remove_fifo();

    /*
     * Destroy components.
     */
    thread_pool_destroy(
        &pool
    );

    monitor_destroy(
        &monitor
    );

    scheduler_destroy(
        &scheduler
    );

    printf(
        "[SERVER] TaskForge stopped\n"
    );

    return 0;
}
