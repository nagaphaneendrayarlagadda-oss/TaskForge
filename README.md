# TaskForge – A Thread-Pool Task Engine

## Project Overview

TaskForge is an operating-system and system-programming project that implements a concurrent task execution engine using a thread pool, priority-based scheduling, inter-process communication (IPC), and task monitoring.

The system provides a command-line client called `taskctl` that communicates with the TaskForge server through a named FIFO.

## Architecture

```text
                    TASKCTL CLIENT
                         |
                         | FIFO IPC
                         v
                 +---------------+
                 |   TASKFORGE   |
                 |    SERVER     |
                 +---------------+
                         |
                         v
                 +---------------+
                 |   SCHEDULER   |
                 | Priority Queue|
                 +---------------+
                         |
                         v
                 +---------------+
                 |  THREAD POOL  |
                 | W1 W2 W3 W4   |
                 +---------------+
                         |
                         v
                    TASK EXECUTION
