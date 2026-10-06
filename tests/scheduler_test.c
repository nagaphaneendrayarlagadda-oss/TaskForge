#include "scheduler.h"

#include <stdio.h>
#include <string.h>

static int test_priority_order(void)
{
    scheduler_t scheduler;

    if (scheduler_init(&scheduler) != 0)
    {
        printf("FAIL: scheduler_init\n");
        return 0;
    }

    task_t low = {
        .id = 1,
        .priority = 2,
        .duration_ms = 1000
    };

    strcpy(low.name, "Low");

    task_t high = {
        .id = 2,
        .priority = 9,
        .duration_ms = 1000
    };

    strcpy(high.name, "High");

    task_t medium = {
        .id = 3,
        .priority = 5,
        .duration_ms = 1000
    };

    strcpy(medium.name, "Medium");

    scheduler_submit(&scheduler, &low);
    scheduler_submit(&scheduler, &high);
    scheduler_submit(&scheduler, &medium);

    task_t result;

    scheduler_get_next(&scheduler, &result);

    if (result.priority != 9)
    {
        printf("FAIL: highest priority was not selected\n");
        scheduler_destroy(&scheduler);
        return 0;
    }

    scheduler_get_next(&scheduler, &result);

    if (result.priority != 5)
    {
        printf("FAIL: second highest priority was not selected\n");
        scheduler_destroy(&scheduler);
        return 0;
    }

    scheduler_get_next(&scheduler, &result);

    if (result.priority != 2)
    {
        printf("FAIL: lowest priority was not selected last\n");
        scheduler_destroy(&scheduler);
        return 0;
    }

    scheduler_destroy(&scheduler);

    printf("PASS: priority ordering\n");

    return 1;
}

static int test_queue_size(void)
{
    scheduler_t scheduler;

    if (scheduler_init(&scheduler) != 0)
    {
        printf("FAIL: scheduler_init\n");
        return 0;
    }

    task_t task = {
        .id = 1,
        .priority = 5,
        .duration_ms = 1000
    };

    strcpy(task.name, "Size-Test");

    if (scheduler_size(&scheduler) != 0)
    {
        printf("FAIL: initial queue size\n");
        scheduler_destroy(&scheduler);
        return 0;
    }

    scheduler_submit(&scheduler, &task);

    if (scheduler_size(&scheduler) != 1)
    {
        printf("FAIL: queue size after submit\n");
        scheduler_destroy(&scheduler);
        return 0;
    }

    scheduler_get_next(&scheduler, &task);

    if (scheduler_size(&scheduler) != 0)
    {
        printf("FAIL: queue size after retrieval\n");
        scheduler_destroy(&scheduler);
        return 0;
    }

    scheduler_destroy(&scheduler);

    printf("PASS: queue size\n");

    return 1;
}

int main(void)
{
    int passed = 0;
    int total = 2;

    printf("====================================\n");
    printf("       SCHEDULER UNIT TEST\n");
    printf("====================================\n");

    if (test_priority_order())
    {
        passed++;
    }

    if (test_queue_size())
    {
        passed++;
    }

    printf("------------------------------------\n");
    printf("Tests passed: %d/%d\n", passed, total);

    if (passed == total)
    {
        printf("ALL SCHEDULER TESTS PASSED\n");
        return 0;
    }

    printf("SOME TESTS FAILED\n");
    return 1;
}
