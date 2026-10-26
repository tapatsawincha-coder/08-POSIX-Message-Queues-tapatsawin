#ifndef MSGQ_LAB_H
#define MSGQ_LAB_H

/* Struct exchanged in every task — must not exceed mq_msgsize when sent */
struct SensorReading {
    int sensor_id;
    int value;
};

/* Task A: send a SensorReading struct from parent to child via POSIX mq */
int task_struct_exchange(void);

/* Task B: priority ordering — highest-priority message dequeued first */
int task_priority_order(void);

/* Task C: queue-full detection — O_NONBLOCK reveals EAGAIN */
int task_full_queue(void);

/* Task D: size enforcement — mq_send rejects payload > mq_msgsize */
int task_size_limit(void);

#endif /* MSGQ_LAB_H */