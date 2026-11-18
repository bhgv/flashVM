/* Copyright (c) 2026 bhgv
 * EN: Native CLI lifecycle and fenced retirement subscription; stdout backpressure never blocks the transport broker.
 * RU: Native CLI lifecycle и fence подписки; backpressure stdout не блокирует transport-брокер.
 */
#include "session.h"
#include "cancel.h"
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>


/* EN/RU: Nonblocking local wake, no transport/provider I/O; a full pipe already represents the sticky wake.
 * Неблокирующее локальное пробуждение, без transport/provider I/O; полная pipe уже содержит sticky-сигнал.
 */
static void wake(int fd)
{
    if (fd >= 0)
    {
        char byte = 1; /* Readiness token, never application data. */
        while (write(fd, &byte, 1) < 0 && errno == EINTR)
        {
        }
    }
}


/* EN/RU: See session.h; first explicit stop wins, including successful receive quota.
 * См. session.h; первый явный stop сохраняется, включая успешную receive-quota.
 */
void unibuscli_stop(UnibusCliSession *session, int error)
{
    pthread_mutex_lock(&session->lock);
    if (!session->stopping)
    {
        session->stopping = 1;
        session->error = error;
    }
    pthread_cond_broadcast(&session->changed);
    pthread_mutex_unlock(&session->lock);
    wake(session->wake[1]);
    wake(session->input_wake[1]);
}


/* EN/RU: Subscription-owned fence outlives session if a native callback was admitted before detachment.
 * Fence принадлежит подписке и переживает session при задержке допущенного native-callback.
 */
typedef struct CliWatch
{
    pthread_mutex_t lock; /* Lifetime fence, acquired before session lock. */
    UnibusCliSession *session; /* Borrowed until detach, NULL afterwards. */
} CliWatch;


/* EN/RU: Mark transport retirement without cancelling ready stdout drain; stdin alone receives this wake.
 * Отметить retirement транспорта без отмены готового stdout-drain; это пробуждение адресовано лишь stdin.
 */
static void retired(void *argument)
{
    CliWatch *watch = argument; /* Callback source retains fence. */
    pthread_mutex_lock(&watch->lock);
    if (watch->session)
    {
        UnibusCliSession *session = watch->session; /* Lifetime fenced through local wake. */
        pthread_mutex_lock(&session->lock);
        session->retired = 1;
        pthread_cond_broadcast(&session->changed);
        pthread_mutex_unlock(&session->lock);
        wake(session->input_wake[1]);
    }
    pthread_mutex_unlock(&watch->lock);
}


/* EN/RU: Dispose independent callback state after every admitted invocation, no session access.
 * Освободить независимый callback-state после всех допущенных вызовов, без доступа к session.
 */
static void release(void *argument)
{
    CliWatch *watch = argument; /* Sole final context owner. */
    pthread_mutex_destroy(&watch->lock);
    free(watch);
}


/* EN/RU: Create owned nonblocking cancellation pipe,1/0; close completed descriptors on failure.
 * Создать собственную неблокирующую cancellation-pipe:1/0; при отказе закрыть созданные descriptors.
 */
static int pipepair(int *descriptors)
{
    if (pipe(descriptors))
    {
        return 0;
    }
    for (unsigned i = 0; i < 2; i++)
    {
        int flags = fcntl(descriptors[i], F_GETFL); /* Preserve existing file-description flags. */
        if (flags < 0 || fcntl(descriptors[i], F_SETFL, flags | O_NONBLOCK) < 0 || fcntl(descriptors[i], F_SETFD, FD_CLOEXEC) < 0)
        {
            close(descriptors[0]);
            close(descriptors[1]);
            descriptors[0] = -1;
            descriptors[1] = -1;
            return 0;
        }
    }
    return 1;
}


/* EN/RU: Own blocked process-control signals on one native thread; main uses SIGUSR1 only after its stop flag is set.
 * Владеть заблокированными process-control signals в native-потоке; main использует SIGUSR1 лишь после stop-флага.
 */
static void *signals(void *argument)
{
    UnibusCliSession *session = argument; /* Borrowed through signal-worker join. */
    sigset_t controls; /* Same mask main installed before connection/native workers existed. */
    sigemptyset(&controls);
    sigaddset(&controls, SIGINT);
    sigaddset(&controls, SIGTERM);
    sigaddset(&controls, SIGUSR1);
    int signal_number; /* Exact signal, no async-unsafe handler enters a mutex or VM. */
    if (sigwait(&controls, &signal_number))
    {
        unibuscli_stop(session, 2);
        return NULL;
    }
    pthread_mutex_lock(&session->lock);
    int stopping = session->stopping; /* Private completion signal is meaningful only during owner cleanup. */
    pthread_mutex_unlock(&session->lock);
    if (signal_number != SIGUSR1 || !stopping)
    {
        unibuscli_stop(session, 128 + signal_number);
    }
    return NULL;
}


/* EN/RU: See session.h; this native process owns stdio for the relay lifetime, restores stdout flags before return.
 * См. session.h; native-процесс владеет stdio во время relay, восстанавливая stdout-flags до возврата.
 */
int unibuscli_run(StyxOrdered *peer, const UnibusCliConfig *config)
{
    UnibusCliSession session = {0}; /* Stack owner retained until all callbacks/workers retire. */
    session.wake[0] = -1;
    session.wake[1] = -1;
    session.input_wake[0] = -1;
    session.input_wake[1] = -1;
    session.peer = peer;
    session.config = config;
    if (pthread_mutex_init(&session.lock, NULL))
    {
        return 2;
    }
    if (pthread_mutex_init(&session.output, NULL))
    {
        pthread_mutex_destroy(&session.lock);
        return 2;
    }
    if (pthread_cond_init(&session.changed, NULL))
    {
        pthread_mutex_destroy(&session.output);
        pthread_mutex_destroy(&session.lock);
        return 2;
    }
    int flags = fcntl(STDOUT_FILENO, F_GETFL); /* Save caller's stdout mode; partial writes must be cancellable. */
    int ready = flags >= 0 && !fcntl(STDOUT_FILENO, F_SETFL, flags | O_NONBLOCK) &&
        pipepair(session.wake) && pipepair(session.input_wake); /* All native wake resources before publication. */
    CliWatch *watch = ready ? calloc(1, sizeof(*watch)) : NULL; /* Private until native subscription adopts it. */
    DVM_CancelTap *tap = NULL; /* Owns callback fence after successful registration. */
    if (watch && !pthread_mutex_init(&watch->lock, NULL))
    {
        watch->session = &session;
        tap = styxorderedwatch(peer, retired, watch, release);
        if (!tap)
        {
            release(watch);
            watch = NULL;
        }
    }
    else
    {
        free(watch);
        watch = NULL;
    }
    pthread_t *outputs = tap ? calloc(config->count, sizeof(*outputs)) : NULL; /* Owned per-tag native workers. */
    UnibusCliReader *readers = tap ? calloc(config->count, sizeof(*readers)) : NULL; /* Stable arguments through join. */
    pthread_t signal_worker; /* Native sigwait owner, no async signal handler or guest pause. */
    int signal_started = tap && !pthread_create(&signal_worker, NULL, signals, &session); /* Retire before restoring caller signal mask. */
    pthread_t input; /* Native stdin producer, if successfully started. */
    int input_started = 0; /* Joined only after successful creation. */
    size_t started = 0; /* Successfully created output workers. */
    if (!outputs || !readers || !signal_started)
    {
        unibuscli_stop(&session, 2);
    }
    else
    {
        for (size_t i = 0; i < config->count; i++)
        {
            readers[i].session = &session;
            readers[i].tag = config->tags[i];
            readers[i].client = styxorderedclient(peer, config->tags[i]);
            if (!readers[i].client || pthread_create(outputs + i, NULL, unibuscli_output, readers + i))
            {
                unibuscli_stop(&session, 2);
                break;
            }
            started++;
        }
        if (pthread_create(&input, NULL, unibuscli_input, &session))
        {
            unibuscli_stop(&session, 2);
        }
        else
        {
            input_started = 1;
        }
    }
    pthread_mutex_lock(&session.lock);
    while (!session.stopping && session.finished < started)
    {
        pthread_cond_wait(&session.changed, &session.lock);
    }
    int stopped = session.stopping; /* Explicit quota/error already decided outcome. */
    pthread_mutex_unlock(&session.lock);
    if (!stopped)
    {
        int reason = styxorderedstate(peer); /* All ready client queues drained before deciding transport outcome. */
        unibuscli_stop(&session, reason == STYX_CONTROL_EOF && !config->records ? 0 : 2);
    }
    styxorderedstop(peer); /* Wake native client waits; never invoked while session mutex is held. */
    if (input_started)
    {
        pthread_join(input, NULL);
    }
    for (size_t i = 0; i < started; i++)
    {
        pthread_join(outputs[i], NULL);
    }
    if (signal_started)
    {
        pthread_kill(signal_worker, SIGUSR1);
        pthread_join(signal_worker, NULL);
    }
    if (watch)
    {
        pthread_mutex_lock(&watch->lock);
        watch->session = NULL;
        pthread_mutex_unlock(&watch->lock);
        dvm_cancel_unsubscribe(tap);
    }
    free(readers);
    free(outputs);
    for (unsigned i = 0; i < 2; i++)
    {
        if (session.wake[i] >= 0)
        {
            close(session.wake[i]);
        }
        if (session.input_wake[i] >= 0)
        {
            close(session.input_wake[i]);
        }
    }
    if (flags >= 0)
    {
        if (fcntl(STDOUT_FILENO, F_SETFL, flags) < 0)
        {
            session.error = 2;
        }
    }
    int result = session.error; /* Every worker and callback access has retired. */
    pthread_cond_destroy(&session.changed);
    pthread_mutex_destroy(&session.output);
    pthread_mutex_destroy(&session.lock);
    return result;
}
