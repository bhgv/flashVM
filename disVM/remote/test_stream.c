/* Copyright (c) 2026 bhgv
 * EN: Real socket/process faults, fragmentation, absolute deadlines and lost replies.
 * RU: Настоящие socket/process faults, fragmentation, deadlines и потеря ответов.
 */
#include "stream.h"
#include "session.h"
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <unistd.h>

static volatile sig_atomic_t interrupts; /* Signal-only evidence that poll was interrupted. */


/* EN/RU: Evaluate condition once and fail with its source expression.
 * Вычислить condition один раз, завершить с исходным выражением при отказе. */
#define CHECK(condition)                                                                                               \
    do                                                                                                                 \
    {                                                                                                                  \
        if (!(condition))                                                                                              \
        {                                                                                                              \
            fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #condition);                                       \
            exit(1);                                                                                                   \
        }                                                                                                              \
    } while (0)


/* EN/RU: Compute an absolute test deadline from the actual monotonic clock.
 * Вычислить абсолютный test deadline по настоящему monotonic clock. */
static uint64_t after(unsigned int milliseconds)
{
    uint64_t now;
    CHECK(dvm_remote_now_ms(&now) == 0);
    return now + milliseconds;
}


/* EN: Join child and verify its real exit status; no guessed process success.
 * RU: Дождаться child и проверить настоящий exit status. */
static void join_child(pid_t child)
{
    int status;
    CHECK(waitpid(child, &status, 0) == child);
    CHECK(WIFEXITED(status) && WEXITSTATUS(status) == 0);
}


/* EN: Child fragments data bytewise; parent must consume exactly the requested
 * length then observe EOF. Closing one endpoint cannot close the borrowed peer.
 * RU: Child дробит data по байту; parent читает точную длину, затем EOF.
 * Закрытие одного endpoint не закрывает заимствованный peer. */
static void fragmented_process(void)
{
    int sockets[2];
    int flags;
    pid_t child;
    unsigned char bytes[32];
    unsigned int index;
    DvmRemoteIoResult result;
    CHECK(socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) == 0);
    child = fork();
    CHECK(child >= 0);
    if (child == 0)
    {
        close(sockets[0]);
        for (index = 0; index < sizeof(bytes); index++)
        {
            unsigned char byte = (unsigned char)index;
            result = dvm_remote_write_exact(sockets[1], &byte, 1, after(2000));
            if (result.status != DVM_REMOTE_IO_OK || poll(NULL, 0, 1) != 0)
            {
                _exit(2);
            }
        }
        close(sockets[1]);
        _exit(0);
    }
    close(sockets[1]);
    flags = fcntl(sockets[0], F_GETFL);
    result = dvm_remote_read_exact(sockets[0], bytes, sizeof(bytes), after(3000));
    CHECK(result.status == DVM_REMOTE_IO_OK && result.transferred == sizeof(bytes));
    for (index = 0; index < sizeof(bytes); index++)
    {
        CHECK(bytes[index] == index);
    }
    result = dvm_remote_read_exact(sockets[0], bytes, 1, after(3000));
    CHECK(result.status == DVM_REMOTE_IO_EOF && result.transferred == 0);
    CHECK(fcntl(sockets[0], F_GETFL) == flags);
    close(sockets[0]);
    join_child(child);
}


/* EN/RU: A truncated process reply retains its exact consumed prefix and EOF.
 * Обрезанный process reply сохраняет точный consumed prefix и EOF. */
static void truncated_process(void)
{
    int sockets[2];
    pid_t child;
    char buffer[8] = {0};
    DvmRemoteIoResult result;
    CHECK(socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) == 0);
    child = fork();
    CHECK(child >= 0);
    if (child == 0)
    {
        close(sockets[0]);
        result = dvm_remote_write_exact(sockets[1], "abc", 3, after(2000));
        close(sockets[1]);
        _exit(result.status == DVM_REMOTE_IO_OK ? 0 : 2);
    }
    close(sockets[1]);
    result = dvm_remote_read_exact(sockets[0], buffer, sizeof(buffer), after(3000));
    CHECK(result.status == DVM_REMOTE_IO_EOF && result.transferred == 3);
    CHECK(memcmp(buffer, "abc", 3) == 0);
    close(sockets[0]);
    join_child(child);
}


/* EN: A trickling peer cannot restart the overall timeout after each byte.
 * RU: Peer с медленными bytes не продлевает общий deadline после каждого byte. */
static void trickle_deadline(void)
{
    int sockets[2];
    pid_t child;
    char bytes[16];
    DvmRemoteIoResult result;
    uint64_t start;
    uint64_t end;
    CHECK(socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) == 0);
    child = fork();
    CHECK(child >= 0);
    if (child == 0)
    {
        unsigned int index;
        close(sockets[0]);
        for (index = 0; index < sizeof(bytes); index++)
        {
            result = dvm_remote_write_exact(sockets[1], "x", 1, after(2000));
            if (result.status != DVM_REMOTE_IO_OK)
            {
                close(sockets[1]);
                _exit(0);
            }
            poll(NULL, 0, 20);
        }
        close(sockets[1]);
        _exit(0);
    }
    close(sockets[1]);
    CHECK(dvm_remote_now_ms(&start) == 0);
    result = dvm_remote_read_exact(sockets[0], bytes, sizeof(bytes), start + 60);
    CHECK(dvm_remote_now_ms(&end) == 0);
    CHECK(result.status == DVM_REMOTE_IO_TIMEOUT && result.transferred < sizeof(bytes));
    CHECK(end - start < 1000);
    close(sockets[0]);
    join_child(child);
}


/* EN: Exercise partial blocked writes, expired/empty/invalid requests and a
 * dead peer without SIGPIPE; caller's descriptor flags remain unchanged.
 * RU: Проверить partial blocked writes, expired/empty/invalid requests и мёртвый
 * peer без SIGPIPE; descriptor flags caller не меняются. */
static void blocked_and_invalid(void)
{
    int sockets[2];
    int small = 4096;
    unsigned char bytes[DVM_REMOTE_IO_LIMIT];
    DvmRemoteIoResult result;
    CHECK(socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) == 0);
    CHECK(setsockopt(sockets[0], SOL_SOCKET, SO_SNDBUF, &small, sizeof(small)) == 0);
    memset(bytes, 0x5a, sizeof(bytes));
    result = dvm_remote_write_exact(sockets[0], bytes, sizeof(bytes), after(30));
    CHECK(result.status == DVM_REMOTE_IO_TIMEOUT);
    CHECK(result.transferred > 0 && result.transferred < sizeof(bytes));
    result = dvm_remote_read_exact(sockets[0], bytes, 1, 0);
    CHECK(result.status == DVM_REMOTE_IO_TIMEOUT && result.transferred == 0);
    result = dvm_remote_read_exact(sockets[0], NULL, 0, 0);
    CHECK(result.status == DVM_REMOTE_IO_OK && result.transferred == 0);
    result = dvm_remote_read_exact(sockets[0], NULL, 1, after(30));
    CHECK(result.status == DVM_REMOTE_IO_INVALID);
    result = dvm_remote_write_exact(sockets[0], bytes, sizeof(bytes) + 1, after(30));
    CHECK(result.status == DVM_REMOTE_IO_INVALID && result.transferred == 0);
    close(sockets[1]);
    result = dvm_remote_write_exact(sockets[0], bytes, 1, after(1000));
    CHECK(result.status == DVM_REMOTE_IO_ERROR && result.transferred == 0);
    CHECK(result.system_error == EPIPE || result.system_error == ECONNRESET);
    close(sockets[0]);
}


/* EN/RU: Signal handler only increments a sig_atomic_t; no unsafe library call.
 * Signal handler только изменяет sig_atomic_t, без небезопасных library calls. */
static void alarm_interrupt(int signal_number)
{
    (void)signal_number;
    interrupts = 1;
}


/* EN: Repeated real EINTR must not extend the overall deadline or fabricate EOF.
 * RU: Настоящие повторные EINTR не продлевают deadline и не превращаются в EOF. */
static void interrupted_deadline(void)
{
    struct sigaction action;
    struct sigaction previous;
    struct itimerval timer;
    int sockets[2];
    char byte;
    DvmRemoteIoResult result;
    CHECK(socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) == 0);
    memset(&action, 0, sizeof(action));
    memset(&timer, 0, sizeof(timer));
    action.sa_handler = alarm_interrupt;
    sigemptyset(&action.sa_mask);
    CHECK(sigaction(SIGALRM, &action, &previous) == 0);
    timer.it_interval.tv_usec = 1000;
    timer.it_value.tv_usec = 1000;
    CHECK(setitimer(ITIMER_REAL, &timer, NULL) == 0);
    result = dvm_remote_read_exact(sockets[0], &byte, 1, after(30));
    memset(&timer, 0, sizeof(timer));
    CHECK(setitimer(ITIMER_REAL, &timer, NULL) == 0);
    CHECK(sigaction(SIGALRM, &previous, NULL) == 0);
    CHECK(interrupts != 0);
    CHECK(result.status == DVM_REMOTE_IO_TIMEOUT && result.transferred == 0);
    close(sockets[0]);
    close(sockets[1]);
}


/* EN: Lost reply after an admitted effect is not cancellation; exact same ID
 * remains completed, and disconnect cleanup waits for admitted root pins.
 * RU: Потеря reply после эффекта не означает отмену; тот же ID остаётся completed,
 * а disconnect cleanup ждёт снятия admitted pins. Native fixture, not a VM call. */
static void lost_reply(void)
{
    int sockets[2];
    int effect = 0;
    int done;
    DvmRemoteSession *session = dvm_remote_session_new(1, 4, 100);
    DvmRemoteRequest request;
    DvmRemoteRoot root;
    DvmRemoteIoResult result;
    memset(&request, 0, sizeof(request));
    CHECK(session != NULL);
    CHECK(dvm_remote_publish(session, DVM_REMOTE_FUNCTION, &effect, &request.target) == DVM_REMOTE_OK);
    request.id = 1;
    request.kind = DVM_REMOTE_FUNCTION;
    CHECK(dvm_remote_admit(session, &request, &done) == DVM_REMOTE_OK);
    effect++;
    CHECK(dvm_remote_finish(session, request.id) == DVM_REMOTE_OK);
    CHECK(socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) == 0);
    close(sockets[1]);
    result = dvm_remote_write_exact(sockets[0], "done", 4, after(1000));
    CHECK(result.status == DVM_REMOTE_IO_ERROR);
    CHECK(dvm_remote_admit(session, &request, &done) == DVM_REMOTE_DUPLICATE && done == 1);
    CHECK(effect == 1);
    close(sockets[0]);
    dvm_remote_session_close(session);
    CHECK(dvm_remote_collect(session, &root) == DVM_REMOTE_OK && root.token == &effect);
    CHECK(dvm_remote_session_destroy(session) == DVM_REMOTE_OK);
}


/* EN/RU: Execute real OS socket/process fault cases; no reference VM execution.
 * Выполнить реальные OS socket/process faults без reference VM execution. */
int main(void)
{
    fragmented_process();
    truncated_process();
    trickle_deadline();
    blocked_and_invalid();
    interrupted_deadline();
    lost_reply();
    puts("PASS remote stream: processes fragments EOF deadlines partial writes lost reply");
    return 0;
}
