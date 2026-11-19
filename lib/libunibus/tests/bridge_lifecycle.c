/* Copyright (c) 2026 bhgv
 * EN: Inject each native worker startup failure into the real gateway helper.
 * Require consumed socket ownership and zero live mutex/condition resources.
 * RU: Внедрять каждый native startup failure в настоящий helper шлюза.
 * Требовать поглощения socket и отсутствия живых mutex/condition ресурсов.
 */
#define _POSIX_C_SOURCE 200809L
#include "bridge.h"
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>

static int stage;       /* 1 fails mutex init, 2 condition init, 3 worker creation. */
static int mutexes;     /* Successfully initialized mutexes not yet destroyed. */
static int conditions;  /* Successfully initialized conditions not yet destroyed. */
static int unexpected;  /* Creation attempted before its prerequisite resource succeeded. */

extern int __real_pthread_mutex_init(pthread_mutex_t *, const pthread_mutexattr_t *);
extern int __real_pthread_mutex_destroy(pthread_mutex_t *);
extern int __real_pthread_cond_init(pthread_cond_t *, const pthread_condattr_t *);
extern int __real_pthread_cond_destroy(pthread_cond_t *);


/* EN: Borrow mutex/attributes. Return EAGAIN without initialization in stage 1;
 * otherwise forward and count only success. Serial test calls only.
 * RU: mutex/attributes заимствованы. Этап 1: EAGAIN без инициализации;
 * иначе настоящий вызов, считать успех. Только последовательные test calls.
 */
int __wrap_pthread_mutex_init(pthread_mutex_t *mutex, const pthread_mutexattr_t *attributes)
{
    if (stage == 1)
    {
        return EAGAIN;
    }
    int result = __real_pthread_mutex_init(mutex, attributes);
    mutexes += result == 0;
    return result;
}


/* EN/RU: Consume initialized mutex through the real API; count successful destruction only.
 * Поглотить mutex настоящим API; считать только успешное уничтожение.
 */
int __wrap_pthread_mutex_destroy(pthread_mutex_t *mutex)
{
    int result = __real_pthread_mutex_destroy(mutex);
    mutexes -= result == 0;
    return result;
}


/* EN: Borrow condition/attributes; stage 2 refuses creation without side effects.
 * Any call in stage 1 means worker startup incorrectly continued after mutex failure.
 * RU: condition/attributes заимствованы; этап 2 отказывает без side effects.
 * Вызов на этапе 1 означает ошибочное продолжение после mutex failure.
 */
int __wrap_pthread_cond_init(pthread_cond_t *condition, const pthread_condattr_t *attributes)
{
    unexpected += stage == 1;
    if (stage == 2)
    {
        return EAGAIN;
    }
    int result = __real_pthread_cond_init(condition, attributes);
    conditions += result == 0;
    return result;
}


/* EN/RU: Consume initialized condition; no worker exists in these failure cases.
 * Поглотить condition; worker в этих ошибочных случаях не создаётся.
 */
int __wrap_pthread_cond_destroy(pthread_cond_t *condition)
{
    int result = __real_pthread_cond_destroy(condition);
    conditions -= result == 0;
    return result;
}


/* EN: thread/attributes/function/argument are borrowed and intentionally unused.
 * Stage 3 returns EAGAIN without a thread; earlier calls violate staged startup.
 * RU: thread/attributes/function/argument заимствованы и не используются.
 * Этап 3 возвращает EAGAIN без потока; более ранний вызов нарушает порядок startup.
 */
int __wrap_pthread_create(pthread_t *thread, const pthread_attr_t *attributes, void *(*function)(void *), void *argument)
{
    (void)thread;
    (void)attributes;
    (void)function;
    (void)argument;
    unexpected += stage != 3;
    return EAGAIN;
}


/* EN: No arguments. Real socketpair endpoints are owned by this test until one
 * transfers to worker_start. Return 1 on leaked ownership/resources, otherwise 0.
 * No threads actually start; a process exit must not hide skipped cleanup.
 * RU: Аргументов нет. Socketpair принадлежит тесту до передачи конца worker_start.
 * Вернуть 1 при утечке владения/ресурсов, иначе 0. Потоки не запускаются;
 * process exit не должен скрыть пропущенный cleanup.
 */
int main(void)
{
    for (stage = 1; stage <= 3; stage++)
    {
        int sockets[2];
        if (socketpair(AF_UNIX, SOCK_STREAM, 0, sockets))
        {
            perror("test socketpair");
            return 1;
        }
        Worker worker;
        int started = bridge_worker_start(&worker, sockets[0], 7, -1);
        errno = 0;
        int closed = fcntl(sockets[0], F_GETFD) < 0 && errno == EBADF;
        close(sockets[1]);
        if (started || !closed || mutexes || conditions || unexpected)
        {
            fprintf(stderr, "stage %d: started=%d closed=%d mutexes=%d conditions=%d unexpected=%d\n",
                    stage, started, closed, mutexes, conditions, unexpected);
            return 1;
        }
    }
    puts("PASS bridge startup: mutex/condition/thread failure consumes socket and releases native resources");
    return 0;
}
