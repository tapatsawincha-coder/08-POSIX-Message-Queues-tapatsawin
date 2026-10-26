#include "msgq_lab.h"

#include <errno.h>
#include <fcntl.h>
#include <mqueue.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#define Q_EXCHANGE "/msgq_lab_exchange"
#define Q_PRIO     "/msgq_lab_prio"
#define Q_FULL     "/msgq_lab_full"
#define Q_SIZE     "/msgq_lab_size"
#define MSG_BUF_SIZE 128

int task_struct_exchange(void) {
    struct mq_attr attr = {0};
    attr.mq_maxmsg = 4;
    attr.mq_msgsize = MSG_BUF_SIZE;
    mq_unlink(Q_EXCHANGE);
    mqd_t mqd = mq_open(Q_EXCHANGE, O_CREAT | O_RDWR, 0660, &attr);
    if (mqd == (mqd_t)-1) { perror("mq_open exchange"); return 1; }

    /* Send before the child can block, using a pipe to report send failure. */
    int ready[2];
    if (pipe(ready) == -1) {
        perror("pipe"); mq_close(mqd); mq_unlink(Q_EXCHANGE); return 1;
    }
    pid_t pid = fork();
    if (pid == -1) {
        perror("fork exchange");
        close(ready[0]); close(ready[1]);
        mq_close(mqd); mq_unlink(Q_EXCHANGE); return 1;
    }
    if (pid == 0) {
        close(ready[1]);
        char ok;
        ssize_t n;
        do { n = read(ready[0], &ok, 1); } while (n == -1 && errno == EINTR);
        close(ready[0]);
        int result = 1;
        if (n == 1 && ok == 1) {
            char buf[MSG_BUF_SIZE];
            unsigned int prio;
            do { n = mq_receive(mqd, buf, sizeof(buf), &prio); }
            while (n == -1 && errno == EINTR);
            if (n == -1) { perror("mq_receive exchange"); }
            else if ((size_t)n != sizeof(struct SensorReading)) {
                fprintf(stderr, "unexpected reading size\n");
            } else {
                struct SensorReading r;
                memcpy(&r, buf, sizeof(r));
                printf("child received: sensor id=%d value=%d\n", r.sensor_id, r.value);
                result = 0;
            }
        }
        if (mq_close(mqd) == -1) { perror("mq_close exchange"); result = 1; }
        if (mq_unlink(Q_EXCHANGE) == -1) { perror("mq_unlink exchange"); result = 1; }
        exit(result ? EXIT_FAILURE : EXIT_SUCCESS);
    }
    close(ready[0]);
    struct SensorReading r = { .sensor_id = 7, .value = 42 };
    int rc;
    do { rc = mq_send(mqd, (const char *)&r, sizeof(r), 0); }
    while (rc == -1 && errno == EINTR);
    int result = rc == -1;
    if (result) { perror("mq_send exchange"); }
    char ok = result ? 0 : 1;
    ssize_t written;
    do { written = write(ready[1], &ok, 1); } while (written == -1 && errno == EINTR);
    close(ready[1]);
    if (written != 1) { perror("write readiness"); result = 1; }
    if (mq_close(mqd) == -1) { perror("mq_close exchange"); result = 1; }
    int status;
    pid_t waited;
    do { waited = waitpid(pid, &status, 0); } while (waited == -1 && errno == EINTR);
    if (waited == -1) { perror("waitpid exchange"); result = 1; }
    else if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) { result = 1; }
    if (result) { mq_unlink(Q_EXCHANGE); return 1; }
    printf("parent: sent reading\n");
    return 0;
}

int task_priority_order(void) {
    struct mq_attr attr = {0};
    attr.mq_maxmsg = 4;
    attr.mq_msgsize = MSG_BUF_SIZE;
    mq_unlink(Q_PRIO);
    mqd_t mqd = mq_open(Q_PRIO, O_CREAT | O_RDWR, 0660, &attr);
    if (mqd == (mqd_t)-1) { perror("mq_open priority"); return 1; }
    struct SensorReading lo = { .sensor_id = 1, .value = 10 };
    struct SensorReading hi = { .sensor_id = 2, .value = 99 };
    if (mq_send(mqd, (const char *)&lo, sizeof(lo), 1) == -1 ||
        mq_send(mqd, (const char *)&hi, sizeof(hi), 5) == -1) {
        perror("mq_send priority");
        mq_close(mqd); mq_unlink(Q_PRIO); return 1;
    }
    if (mq_close(mqd) == -1) {
        perror("mq_close priority"); mq_unlink(Q_PRIO); return 1;
    }
    fflush(stdout);
    pid_t pid = fork();
    if (pid == -1) { perror("fork priority"); mq_unlink(Q_PRIO); return 1; }
    if (pid == 0) {
        mqd = mq_open(Q_PRIO, O_RDONLY);
        if (mqd == (mqd_t)-1) {
            perror("mq_open priority child"); mq_unlink(Q_PRIO); exit(EXIT_FAILURE);
        }
        int result = 0;
        for (int i = 0; i < 2; ++i) {
            char buf[MSG_BUF_SIZE];
            unsigned int prio;
            ssize_t n;
            do { n = mq_receive(mqd, buf, sizeof(buf), &prio); }
            while (n == -1 && errno == EINTR);
            if (n == -1) { perror("mq_receive priority"); result = 1; break; }
            if ((size_t)n != sizeof(struct SensorReading)) {
                fprintf(stderr, "unexpected reading size\n"); result = 1; break;
            }
            struct SensorReading r;
            memcpy(&r, buf, sizeof(r));
            printf("child received %s-pri: value=%d\n", i == 0 ? "hi" : "lo", r.value);
        }
        if (mq_close(mqd) == -1) { perror("mq_close priority"); result = 1; }
        if (mq_unlink(Q_PRIO) == -1) { perror("mq_unlink priority"); result = 1; }
        exit(result ? EXIT_FAILURE : EXIT_SUCCESS);
    }
    int status;
    pid_t waited;
    do { waited = waitpid(pid, &status, 0); } while (waited == -1 && errno == EINTR);
    if (waited == -1) { perror("waitpid priority"); mq_unlink(Q_PRIO); return 1; }
    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
        mq_unlink(Q_PRIO); return 1;
    }
    return 0;
}

int task_full_queue(void) {
    struct mq_attr attr = {0};
    attr.mq_maxmsg = 2;
    attr.mq_msgsize = MSG_BUF_SIZE;
    mq_unlink(Q_FULL);
    mqd_t mqd = mq_open(Q_FULL, O_CREAT | O_RDWR | O_NONBLOCK, 0660, &attr);
    if (mqd == (mqd_t)-1) { perror("mq_open full"); return 1; }
    struct SensorReading r = { .sensor_id = 7, .value = 42 };
    int result = 1;
    if (mq_send(mqd, (const char *)&r, sizeof(r), 0) == -1 ||
        mq_send(mqd, (const char *)&r, sizeof(r), 0) == -1) {
        perror("mq_send fill");
    } else {
        int rc = mq_send(mqd, (const char *)&r, sizeof(r), 0);
        if (rc == -1 && errno == EAGAIN) {
            printf("queue full: EAGAIN detected\n"); result = 0;
        } else if (rc == -1) { perror("mq_send full"); }
        else { fprintf(stderr, "expected EAGAIN but send succeeded\n"); }
    }
    if (mq_close(mqd) == -1) { perror("mq_close full"); result = 1; }
    if (mq_unlink(Q_FULL) == -1) { perror("mq_unlink full"); result = 1; }
    return result;
}

int task_size_limit(void) {
    struct mq_attr attr = {0};
    attr.mq_maxmsg = 4;
    attr.mq_msgsize = sizeof(struct SensorReading);
    mq_unlink(Q_SIZE);
    mqd_t mqd = mq_open(Q_SIZE, O_CREAT | O_WRONLY, 0660, &attr);
    if (mqd == (mqd_t)-1) { perror("mq_open size"); return 1; }
    char oversized[sizeof(struct SensorReading) + 1] = {0};
    int result = 1;
    int rc = mq_send(mqd, oversized, sizeof(oversized), 0);
    if (rc == -1 && errno == EMSGSIZE) {
        printf("oversized send: EMSGSIZE detected\n"); result = 0;
    } else if (rc == -1) { perror("mq_send size"); }
    else { fprintf(stderr, "expected EMSGSIZE but send succeeded\n"); }
    if (mq_close(mqd) == -1) { perror("mq_close size"); result = 1; }
    if (mq_unlink(Q_SIZE) == -1) { perror("mq_unlink size"); result = 1; }
    return result;
}

int main(void) {
    if (task_struct_exchange() != 0) return EXIT_FAILURE;
    if (task_priority_order()  != 0) return EXIT_FAILURE;
    if (task_full_queue()      != 0) return EXIT_FAILURE;
    if (task_size_limit()      != 0) return EXIT_FAILURE;
    printf("all tasks done\n");
    return EXIT_SUCCESS;
}