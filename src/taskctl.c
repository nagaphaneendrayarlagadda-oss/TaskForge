#include "ipc.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static void print_usage(const char *program)
{
    printf("Usage:\n");
    printf("  %s submit <priority> <duration_ms> <name>\n", program);
    printf("  %s status\n", program);
    printf("  %s shutdown\n", program);
}

int main(int argc, char *argv[])
{
    ipc_message_t message;

    memset(
        &message,
        0,
        sizeof(message)
    );

    if (argc < 2)
    {
        print_usage(argv[0]);
        return 1;
    }

    /*
     * SUBMIT command
     */
    if (strcmp(argv[1], "submit") == 0)
    {
        if (argc != 5)
        {
            print_usage(argv[0]);
            return 1;
        }

        message.command = IPC_CMD_SUBMIT;

        /*
         * Client PID is used by the server
         * to send the response back.
         */
        message.client_pid = getpid();

        message.task.id = 0;

        message.task.priority =
            atoi(argv[2]);

        message.task.duration_ms =
            atoi(argv[3]);

        strncpy(
            message.task.name,
            argv[4],
            TASK_NAME_MAX - 1
        );

        message.task.name[
            TASK_NAME_MAX - 1
        ] = '\0';

        /*
         * Validate priority.
         */
        if (message.task.priority < 1 ||
            message.task.priority > 10)
        {
            fprintf(
                stderr,
                "Priority must be between 1 and 10\n"
            );

            return 1;
        }

        /*
         * Validate duration.
         */
        if (message.task.duration_ms <= 0)
        {
            fprintf(
                stderr,
                "Duration must be greater than 0\n"
            );

            return 1;
        }

        char response_path[128];

        /*
         * Create a private response FIFO
         * for this client.
         */
        ipc_response_fifo_path(
            message.client_pid,
            response_path,
            sizeof(response_path)
        );

        unlink(response_path);

        if (mkfifo(response_path, 0666) == -1)
        {
            perror("mkfifo response");

            return 1;
        }

        /*
         * Send submit request to server.
         */
        if (ipc_client_send(&message) != 0)
        {
            unlink(response_path);

            return 1;
        }

        /*
         * Wait for server response.
         */
        ipc_response_t response;

        memset(
            &response,
            0,
            sizeof(response)
        );

        if (ipc_client_receive(
                message.client_pid,
                &response) != 0)
        {
            unlink(response_path);

            return 1;
        }

        /*
         * Remove the private response FIFO.
         */
        unlink(response_path);

        /*
         * Display server response.
         */
        if (response.success)
        {
            printf(
                "Task submitted successfully!\n"
            );

            printf(
                "Job ID: %d\n",
                response.task_id
            );

            printf(
                "Priority: %d\n",
                message.task.priority
            );

            printf(
                "Duration: %d ms\n",
                message.task.duration_ms
            );

            printf(
                "Name: %s\n",
                message.task.name
            );
        }
        else
        {
            fprintf(
                stderr,
                "Task submission failed.\n"
            );

            if (response.text[0] != '\0')
            {
                fprintf(
                    stderr,
                    "%s",
                    response.text
                );
            }

            return 1;
        }

        return 0;
    }

    /*
     * STATUS command
     */
    if (strcmp(argv[1], "status") == 0)
    {
        if (argc != 2)
        {
            print_usage(argv[0]);
            return 1;
        }

        char response_path[128];

        message.command = IPC_CMD_STATUS;

        message.client_pid = getpid();

        /*
         * Create a private response FIFO
         * for this client.
         */
        ipc_response_fifo_path(
            message.client_pid,
            response_path,
            sizeof(response_path)
        );

        unlink(response_path);

        if (mkfifo(response_path, 0666) == -1)
        {
            perror("mkfifo response");

            return 1;
        }

        /*
         * Send status request to server.
         */
        if (ipc_client_send(&message) != 0)
        {
            unlink(response_path);

            return 1;
        }

        /*
         * Wait for the server response.
         */
        ipc_response_t response;

        memset(
            &response,
            0,
            sizeof(response)
        );

        if (ipc_client_receive(
                message.client_pid,
                &response) != 0)
        {
            unlink(response_path);

            return 1;
        }

        printf(
            "%s",
            response.text
        );

        unlink(response_path);

        return response.success ? 0 : 1;
    }

    /*
     * SHUTDOWN command
     */
    if (strcmp(argv[1], "shutdown") == 0)
    {
        if (argc != 2)
        {
            print_usage(argv[0]);
            return 1;
        }

        message.command = IPC_CMD_SHUTDOWN;

        if (ipc_client_send(&message) != 0)
        {
            return 1;
        }

        printf(
            "Shutdown request sent.\n"
        );

        return 0;
    }

    fprintf(
        stderr,
        "Unknown command: %s\n",
        argv[1]
    );

    print_usage(argv[0]);

    return 1;
}
