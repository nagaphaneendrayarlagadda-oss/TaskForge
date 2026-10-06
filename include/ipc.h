#ifndef IPC_H
#define IPC_H

#include "task.h"

#include <sys/types.h>

#define TASKFORGE_FIFO "/tmp/taskforge_fifo"

#define IPC_CMD_SUBMIT   1
#define IPC_CMD_STATUS   2
#define IPC_CMD_SHUTDOWN 3

#define IPC_RESPONSE_TEXT 2048

typedef struct
{
    int command;

    pid_t client_pid;

    task_t task;

} ipc_message_t;

typedef struct
{
    int success;

    int task_id;

    char text[IPC_RESPONSE_TEXT];

} ipc_response_t;

int ipc_server_create(void);

int ipc_server_open(void);

int ipc_server_read(
    int fd,
    ipc_message_t *message
);

void ipc_server_close(int fd);

int ipc_client_send(
    const ipc_message_t *message
);

int ipc_client_receive(
    pid_t client_pid,
    ipc_response_t *response
);

void ipc_remove_fifo(void);

void ipc_response_fifo_path(
    pid_t client_pid,
    char *path,
    size_t path_size
);

#endif
