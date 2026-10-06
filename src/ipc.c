#include "ipc.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

/*
 * Build the response FIFO path for a client.
 */
void ipc_response_fifo_path(
    pid_t client_pid,
    char *path,
    size_t path_size)
{
    if (path == NULL || path_size == 0)
        return;

    snprintf(
        path,
        path_size,
        "/tmp/taskforge_response_%d",
        (int)client_pid
    );
}

/*
 * Create the main TaskForge named FIFO.
 */
int ipc_server_create(void)
{
    /*
     * Remove an old FIFO if it exists.
     */
    unlink(TASKFORGE_FIFO);

    /*
     * Create named FIFO.
     */
    if (mkfifo(TASKFORGE_FIFO, 0666) == -1)
    {
        perror("mkfifo");
        return -1;
    }

    return 0;
}

/*
 * Open the server FIFO.
 */
int ipc_server_open(void)
{
    int fd;

    fd = open(
        TASKFORGE_FIFO,
        O_RDWR
    );

    if (fd == -1)
    {
        perror("open FIFO");
        return -1;
    }

    return fd;
}

/*
 * Read an IPC message from the server FIFO.
 */
int ipc_server_read(
    int fd,
    ipc_message_t *message)
{
    if (message == NULL)
        return -1;

    ssize_t bytes_read = read(
        fd,
        message,
        sizeof(ipc_message_t)
    );

    if (bytes_read == 0)
    {
        return 0;
    }

    if (bytes_read == -1)
    {
        if (errno == EINTR)
            return 0;

        perror("read FIFO");
        return -1;
    }

    if ((size_t)bytes_read != sizeof(ipc_message_t))
    {
        fprintf(
            stderr,
            "Incomplete IPC message\n"
        );

        return -1;
    }

    return 1;
}

/*
 * Close the server FIFO.
 */
void ipc_server_close(int fd)
{
    if (fd >= 0)
    {
        close(fd);
    }
}

/*
 * Send a message from taskctl to TaskForge server.
 */
int ipc_client_send(
    const ipc_message_t *message)
{
    if (message == NULL)
        return -1;

    int fd = open(
        TASKFORGE_FIFO,
        O_WRONLY
    );

    if (fd == -1)
    {
        fprintf(
            stderr,
            "Cannot connect to TaskForge server. "
            "Is the server running?\n"
        );

        return -1;
    }

    ssize_t bytes_written = write(
        fd,
        message,
        sizeof(ipc_message_t)
    );

    if (bytes_written != sizeof(ipc_message_t))
    {
        perror("write FIFO");

        close(fd);

        return -1;
    }

    close(fd);

    return 0;
}

/*
 * Receive a response from the TaskForge server.
 */
int ipc_client_receive(
    pid_t client_pid,
    ipc_response_t *response)
{
    if (response == NULL)
        return -1;

    char path[128];

    ipc_response_fifo_path(
        client_pid,
        path,
        sizeof(path)
    );

    int fd = open(
        path,
        O_RDONLY
    );

    if (fd == -1)
    {
        perror("open response FIFO");
        return -1;
    }

    ssize_t bytes_read = read(
        fd,
        response,
        sizeof(ipc_response_t)
    );

    close(fd);

    if (bytes_read != sizeof(ipc_response_t))
    {
        fprintf(
            stderr,
            "Incomplete response received\n"
        );

        return -1;
    }

    return 0;
}

/*
 * Remove the main TaskForge FIFO.
 */
void ipc_remove_fifo(void)
{
    unlink(TASKFORGE_FIFO);
}
