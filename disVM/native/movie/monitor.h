/* Copyright (c) 2026 bhgv
 * EN: Autonomous native control reader with bounded state readiness, cancellation and session ownership.
 * RU: Autonomous native control reader с bounded state readiness, cancellation и session ownership.
 */
#ifndef STYX_MOVIE_MONITOR_H
#define STYX_MOVIE_MONITOR_H
#include "receiver.h"
typedef struct StyxMovieMonitor StyxMovieMonitor; /* Owned native I/O worker; never a guest thread. */
#define STYX_MOVIE_READING 0 /* Control reader is live, pending or ready. */
#define STYX_MOVIE_CLOSED 1 /* Receiver retired by CLOSED or terminal link observation. */
#define STYX_MOVIE_EOF 2 /* Unexpected ordinary control EOF, receiver retired. */
#define STYX_MOVIE_FAILED 3 /* Transport, framing or native dispatch refusal. */
#define STYX_MOVIE_STOPPED 4 /* Host requested cancellation; receiver retired. */


/* EN/RU: Native fragment dispatcher; optional only for a dedicated announcement-only direction.
 * Receive borrows raw byte fragments on the I/O worker, returns1 accepted/0 fail closed.
 * Copy into bounded native queues for other consumers; no VM/SDL, blocking work or reentry.
 * Dispatchers validate complete records before using payloads. State adopted only on successful start, release after worker joins; neither callback is on a guest.
 * Native dispatcher borrowed fragments:1/0, без VM/SDL/blocking/reentry; state принят при success,
 * release после worker join. Для общего control direction callback обязателен по контракту host. */
typedef struct StyxMovieInput
{
    int (*receive)(void *, const void *, size_t); /* Optional native raw-fragment dispatch, bindings included. */
    void *state; /* Adopted only on success, immutable pointer until release. */
    void (*release)(void *); /* Optional final native context cleanup after all receives retire. */
} StyxMovieInput;


/* EN/RU: Copied latest state, not an event queue or authority grant; intermediate states may coalesce.
 * Копия latest state, не event queue/authority grant; intermediate states могут объединяться. */
typedef struct StyxMovieMonitorState
{
    unsigned long long generation; /* Latest receiver generation, initially0. */
    int available; /*1 ready,0 pending,-1 retired. */
    int reason; /* READING while live, otherwise one terminal reason above. */
} StyxMovieMonitorState;


/* EN/RU: Walk/open relative control file on session's exact granted connection, start owned reader/NULL.
 * Borrow session/path/input through return; retain session on success, native setup may block.
 * Requires watchable fd client with >=2 RPC slots and one exclusive input owner; opens read-only.
 * No dial, authority lookup or implicit fallback. Failure unclaims input and preserves caller ownership; transport errors keep ordinary link behavior.
 * Пройти/open relative control на connection session; свой reader/NULL, retain session при success.
 * Native setup может блокировать; нужен watchable fd client, >=2 RPC slots, exclusive input owner. */
StyxMovieMonitor *styxmoviecontrolstart(StyxMovieSession *session, const char *relative,
    const StyxMovieInput *input);


/* EN/RU: Borrow readiness fd/-1 while monitor lives, never read/close directly; use state() to acknowledge.
 * Заимствовать readiness fd/-1; не читать/закрывать напрямую, подтверждать через state(). */
int styxmoviecontrolfd(StyxMovieMonitor *monitor);


/* EN/RU: Copy latest state and drain only native readiness under mutex;1/0 invalid, no transport/guest wait.
 * Caller pins monitor; one state consumer, may race worker/stop. Re-register only while READING.
 * Копировать state и подтвердить readiness;1/0, native mutex, один consumer, без guest wait. */
int styxmoviecontrolstate(StyxMovieMonitor *monitor, StyxMovieMonitorState *out);


/* EN/RU: Nonwaiting native cancellation request, idempotent; no guest participation or shutdown of client.
 * Stop retires the receiver, preventing future stale fetches. Caller pins monitor through request.
 * Идемпотентный native cancel без guest participation/client shutdown; receiver завершается. */
void styxmoviecontrolstop(StyxMovieMonitor *monitor);


/* EN/RU: Consume after external state/fetch callers retire;1 consumed/0 join failure leaves owned.
 * Request stop and join only owned I/O worker.
 * May wait for native flush grace, never for a guest; no self-call from receive/release callbacks.
 * Shared client stays owned by host; native fid cleanup follows existing clunk-later rules.
 * Поглотить после external users; join только собственного I/O worker, возможен native flush grace.
 * Не вызывать из receive/release callbacks; shared client остаётся у host. */
int styxmoviecontrolfree(StyxMovieMonitor *monitor);
#endif
