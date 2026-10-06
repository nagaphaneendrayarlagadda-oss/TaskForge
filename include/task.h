#ifndef TASK_H
#define TASK_H

#define TASK_NAME_MAX 64

typedef struct
{
    int id;
    int priority;
    int duration_ms;
    char name[TASK_NAME_MAX];
} task_t;

#endif
