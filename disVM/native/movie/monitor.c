/* Copyright (c) 2026 bhgv
 * EN: One autonomous native reader feeds a retained session and publishes bounded local readiness.
 * RU: Один autonomous native reader питает retained session и публикует bounded local readiness.
 */
#include "monitor.h"
#include "protocol.h"
#include "session_internal.h"
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>


/* EN/RU: Native worker owns fid/read offset; host owns final stop/join and descriptor acknowledgement.
 * Native worker владеет fid/read offset; host владеет final stop/join и readiness acknowledgement. */
struct StyxMovieMonitor
{
    pthread_mutex_t lock; /* Protects state/stopping and wake-pipe level, never held for RPC/callbacks. */
    pthread_t worker; /* Exactly one native I/O reader, never a VM executor. */
    StyxMovieSession *session; /* Owned public session pin through final worker join. */
    StyxFid *fid; /* Worker-owned control fid after successful thread start. */
    StyxCancel *cancel; /* Borrowed active one-shot token under lock; worker owns its lifetime. */
    StyxMovieInput input; /* Optional host native dispatcher, adopted on successful start only. */
    StyxMovieMonitorState state; /* Latest coherent receiver state, readiness may coalesce changes. */
    int wake[2]; /* Owned nonblocking CLOEXEC readiness pipe. */
    int stopping; /* Sticky host request, protected by lock. */
};


/* EN/RU: Signal readiness under monitor lock or private construction; a full pipe already means ready.
 * Сигнализировать readiness под monitor lock; full pipe уже означает ready. */
static void signalstate(StyxMovieMonitor *monitor)
{
    char byte = 1;
    ssize_t count;
    do
    {
        count = write(monitor->wake[1], &byte, 1);
    } while (count < 0 && errno == EINTR);
}


/* EN/RU: Snapshot receiver outside monitor lock, then publish changed local state/terminal reason.
 * No VM/SDL/provider I/O; reason borrowed value, STOPPED wins once requested while live.
 * Snapshot receiver вне monitor lock, затем publish changed state/reason; без VM/SDL/provider I/O. */
static void publish(StyxMovieMonitor *monitor, int reason)
{
    StyxMovieMonitorState state;
    state.available = styxmovieavailable(monitor->session, &state.generation);
    state.reason = reason;
    pthread_mutex_lock(&monitor->lock);
    if (monitor->stopping)
    {
        state.reason = STYX_MOVIE_STOPPED;
        state.available = -1;
    }
    if (state.available != monitor->state.available || state.reason != monitor->state.reason ||
        state.generation != monitor->state.generation)
    {
        monitor->state = state;
        signalstate(monitor);
    }
    pthread_mutex_unlock(&monitor->lock);
}


/* EN/RU: Observe sticky host request on owned monitor; no callbacks/transport/guest access.
 * Проверить sticky host request под owned monitor; без callbacks/transport/guest access. */
static int stopping(StyxMovieMonitor *monitor)
{
    pthread_mutex_lock(&monitor->lock);
    int stopped = monitor->stopping;
    pthread_mutex_unlock(&monitor->lock);
    return stopped;
}


/* EN/RU: Publish a fresh one-operation token before each read; stop cannot race token disposal.
 * Return1 read completed,0 stopped before admission,-1 allocation failure; buffers borrowed.
 * Свежий одноразовый token для каждого read; stop не гоняется с освобождением.
 * Return1 completed/0 stopped/-1 allocation failure; buffers borrowed. */
static int readnext(StyxMovieMonitor *monitor, unsigned long long offset,
    void *bytes, size_t size, StyxTransfer *transfer)
{
    StyxCancel *cancel = styxcancelcreate();
    if (!cancel)
    {
        return -1;
    }
    pthread_mutex_lock(&monitor->lock);
    if (monitor->stopping)
    {
        pthread_mutex_unlock(&monitor->lock);
        styxcancelfree(cancel);
        return 0;
    }
    monitor->cancel = cancel;
    pthread_mutex_unlock(&monitor->lock);
    styxclientreadwatch(monitor->fid, offset, bytes, size, cancel, transfer);
    pthread_mutex_lock(&monitor->lock);
    monitor->cancel = NULL;
    pthread_mutex_unlock(&monitor->lock);
    styxcancelfree(cancel);
    return 1;
}


/* EN/RU: Own all long-lived reads and input callbacks on this native thread; bounded frame storage.
 * Cancellation uses native Tflush fencing; every exit retires the receiver and independent fid.
 * Все reads/input callbacks на этом native thread; bounded storage, native Tflush при cancel.
 * Любой выход завершает receiver и independent fid, без остановки guest. */
static void *reading(void *state)
{
    StyxMovieMonitor *monitor = state;
    unsigned char bytes[MOVIE_FRAME];
    unsigned long long offset = 0;
    int reason = STYX_MOVIE_CLOSED;
    while (!stopping(monitor) && styxmovieavailable(monitor->session, NULL) >= 0)
    {
        StyxTransfer transfer;
        int read = readnext(monitor, offset, bytes, sizeof(bytes), &transfer);
        if (read <= 0)
        {
            reason = read ? STYX_MOVIE_FAILED : STYX_MOVIE_STOPPED;
            break;
        }
        if (!transfer.success || !transfer.confirmed || transfer.uncertain || transfer.count > sizeof(bytes))
        {
            reason = STYX_MOVIE_FAILED;
            break;
        }
        if (!transfer.count)
        {
            reason = STYX_MOVIE_EOF;
            break;
        }
        if (offset > ~0ULL-transfer.count)
        {
            reason = STYX_MOVIE_FAILED;
            break;
        }
        offset += transfer.count;
        if (monitor->input.receive && monitor->input.receive(monitor->input.state, bytes, transfer.count) != 1)
        {
            reason = STYX_MOVIE_FAILED;
            break;
        }
        if (!movie_session_feed(monitor->session, bytes, transfer.count))
        {
            reason = STYX_MOVIE_FAILED;
            break;
        }
        publish(monitor, STYX_MOVIE_READING);
    }
    styxmoviecontrolend(monitor->session);
    styxclientclunklater(monitor->fid);
    monitor->fid = NULL;
    publish(monitor, reason);
    return NULL;
}


/* EN/RU: Dispose only private setup after no worker was started, or after final join.
 * Input state is released separately only after successful admission, never on constructor failure.
 * Освободить private setup без worker либо после join; input state отдельно, не при failure setup. */
static void dispose(StyxMovieMonitor *monitor)
{
    if (monitor->fid)
    {
        styxclientclunklater(monitor->fid);
    }
    if (monitor->wake[0] >= 0)
    {
        close(monitor->wake[0]);
    }
    if (monitor->wake[1] >= 0)
    {
        close(monitor->wake[1]);
    }
    movie_session_claim(monitor->session, 0);
    styxmoviesessionfree(monitor->session);
    pthread_mutex_destroy(&monitor->lock);
    free(monitor);
}


/* EN/RU: See monitor.h; exclusive feed is reserved before fallible native transport setup.
 * См. monitor.h; exclusive feed резервируется до fallible native transport setup. */
StyxMovieMonitor *styxmoviecontrolstart(StyxMovieSession *session, const char *relative,
    const StyxMovieInput *input)
{
    if (!session || !relative || !*relative || !styxmoviesessionretain(session))
    {
        return NULL;
    }
    StyxClient *client = movie_session_client(session);
    if (!styxclientwatchable(client) || !movie_session_claim(session, 1))
    {
        styxmoviesessionfree(session);
        return NULL;
    }
    StyxMovieMonitor *monitor = calloc(1, sizeof(*monitor));
    if (!monitor || pthread_mutex_init(&monitor->lock, NULL))
    {
        free(monitor);
        movie_session_claim(session, 0);
        styxmoviesessionfree(session);
        return NULL;
    }
    monitor->session = session;
    monitor->wake[0] = monitor->wake[1] = -1;
    if (pipe(monitor->wake) ||
        fcntl(monitor->wake[0], F_SETFL, O_NONBLOCK) || fcntl(monitor->wake[1], F_SETFL, O_NONBLOCK) ||
        fcntl(monitor->wake[0], F_SETFD, FD_CLOEXEC) || fcntl(monitor->wake[1], F_SETFD, FD_CLOEXEC))
    {
        dispose(monitor);
        return NULL;
    }
    monitor->fid = styxclientwalk(client, relative);
    if (!monitor->fid || styxclientfiddirectory(monitor->fid) != 0 || !styxclientopen(monitor->fid, 0))
    {
        dispose(monitor);
        return NULL;
    }
    if (input)
    {
        monitor->input = *input;
    }
    monitor->state.available = styxmovieavailable(session, &monitor->state.generation);
    monitor->state.reason = STYX_MOVIE_READING;
    signalstate(monitor);
    if (pthread_create(&monitor->worker, NULL, reading, monitor))
    {
        dispose(monitor);
        return NULL;
    }
    return monitor;
}


/* EN/RU: See monitor.h; descriptor lifetime is pinned by the caller's monitor owner.
 * См. monitor.h; descriptor lifetime удерживается caller monitor owner. */
int styxmoviecontrolfd(StyxMovieMonitor *monitor)
{
    return monitor ? monitor->wake[0] : -1;
}


/* EN/RU: See monitor.h; draining under the signal lock closes the snapshot/next-wakeup race.
 * См. monitor.h; drain под signal lock закрывает snapshot/next-wakeup race. */
int styxmoviecontrolstate(StyxMovieMonitor *monitor, StyxMovieMonitorState *out)
{
    if (!monitor || !out)
    {
        return 0;
    }
    pthread_mutex_lock(&monitor->lock);
    char bytes[128];
    ssize_t count;
    do
    {
        count = read(monitor->wake[0], bytes, sizeof(bytes));
    } while (count > 0 || (count < 0 && errno == EINTR));
    *out = monitor->state;
    pthread_mutex_unlock(&monitor->lock);
    return 1;
}


/* EN/RU: See monitor.h; publish sticky stop and cancel the borrowed active token under its lifetime lock.
 * См. monitor.h; publish sticky stop и native cancel под token lifetime lock. */
void styxmoviecontrolstop(StyxMovieMonitor *monitor)
{
    if (monitor)
    {
        pthread_mutex_lock(&monitor->lock);
        if (monitor->state.reason == STYX_MOVIE_READING && !monitor->stopping)
        {
            monitor->stopping = 1;
            monitor->state.reason = STYX_MOVIE_STOPPED;
            monitor->state.available = -1;
            signalstate(monitor);
        }
        styxcancelrequest(monitor->cancel);
        pthread_mutex_unlock(&monitor->lock);
        styxmoviecontrolend(monitor->session);
    }
}


/* EN/RU: See monitor.h; failed/self join leaves monitor owned, without freeing worker storage.
 * См. monitor.h; failed/self join оставляет monitor owned, worker storage не освобождается. */
int styxmoviecontrolfree(StyxMovieMonitor *monitor)
{
    if (!monitor)
    {
        return 1;
    }
    if (pthread_equal(pthread_self(), monitor->worker))
    {
        return 0;
    }
    styxmoviecontrolstop(monitor);
    if (pthread_join(monitor->worker, NULL))
    {
        return 0;
    }
    if (monitor->input.release)
    {
        monitor->input.release(monitor->input.state);
    }
    dispose(monitor);
    return 1;
}
