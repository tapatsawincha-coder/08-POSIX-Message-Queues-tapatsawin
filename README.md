# System Programming Lab: POSIX Message Queues — Struct Exchange and Limitations

## 1. Learning Objectives
By the end of this lab, you should be able to:
- exchange a C struct between two processes using `mq_open` / `mq_send` / `mq_receive`
- explain why `mq_msgsize` must be ≥ `sizeof` the struct you intend to send
- observe that POSIX message queues deliver messages **highest-priority-first**, not FIFO
- detect a full queue at send time using `O_NONBLOCK` and `errno == EAGAIN`
- enforce and observe the hard message-size limit (`errno == EMSGSIZE`)
- clean up a named queue with `mq_close` / `mq_unlink`

## 2. Repository Layout
- `src/msgq_lab.c` — source file you must edit (contains TODO sections)
- `include/msgq_lab.h` — struct definition and function prototypes — do not change
- `scripts/` — test and grading scripts
- `tests/` — description of visible checks
- `samples/` — example struct field values for reference

## 3. What You Need To Implement
Complete the four TODO functions in `src/msgq_lab.c`.

### Task A — `task_struct_exchange()`
Send a `SensorReading` struct from a parent process to a child process through a POSIX message queue.

### Task B — `task_priority_order()`
Show that POSIX message queues are priority-ordered, not strictly FIFO.

### Task C — `task_full_queue()`
Detect a full queue using O_NONBLOCK and EAGAIN.

### Task D — `task_size_limit()`
Enforce and observe the hard message-size limit (EMSGSIZE).
