/* Copyright (c) 2026 bhgv
 * EN: Autonomous one-request native VFS resource server on a granted bidirectional S-record route.
 * RU: Автономный native VFS resource server одного запроса по выданному bidir S-маршруту.
 */
#ifndef STYX_BIDIR_MOVIE_SERVER_H
#define STYX_BIDIR_MOVIE_SERVER_H
#include "route/types.h"
#include "namespace.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct StyxBidirMovieServer StyxBidirMovieServer; /* Owned native worker and prepared namespace service pin. */
#define STYX_MOVIE_SERVER_WAITING 0 /* Waiting for one request, no provider operation admitted. */
#define STYX_MOVIE_SERVER_WORKING 1 /* Valid request admitted, VFS/response processing active. */
#define STYX_MOVIE_SERVER_DONE 2 /* Success END copied to the shared TX queue, not peer execution ACK. */
#define STYX_MOVIE_SERVER_REFUSED 3 /* Provider refusal END copied to TX, not a transport failure. */
#define STYX_MOVIE_SERVER_STOPPED 4 /* Explicit host cancellation; worker may still retire native I/O. */
#define STYX_MOVIE_SERVER_FAILED 5 /* Timeout, invalid request, local I/O/protocol/TX failure. */


/* EN: Retain borrowed prepared group in an actual VFS service and start one owned native worker.
 * Return owned server/NULL; failure preserves caller ownership and starts no surviving worker.
 * Borrow route until all server users and final free retire. index is an exclusive resource RX
 * queue matching MOVIE_TYPE/VERSION; tx_tag is independently granted. Limit >= MOVIE_FRAME+32,
 * timeout 1..120000ms for request wait and response combined. No transport setup or new binding.
 * The live namespace is retained, not copied/frozen. Existing service makes final VFS decisions.
 * Route retirement automatically requests this server's native provider scope; FAILED differs from
 * explicit host STOPPED. Providers still honour/bound their own I/O. Subscription failure rolls back.
 * Callback state retains only its cancellation source, never a server/route pointer.
 * One request only; host services other queues and retires this logical channel before reuse.
 * RU: Удержать prepared group и запустить один native worker, свой server/NULL. Route заимствован
 * до free, namespace остаётся живым. Один запрос, права определяет существующий VFS service.
 */
StyxBidirMovieServer *styxbidirmovieserver(StyxBidirRoute *route, unsigned index,
    unsigned long tx_tag, DVM_Namespace *group, unsigned timeout);


/* EN/RU: Snapshot WAITING..FAILED/-1 invalid, no I/O or queue consumption; caller pins server.
 * Скопировать status, -1 invalid, без I/O и потребления очередей; caller удерживает server.
 */
int styxbidirmovieserverstate(StyxBidirMovieServer *server);


/* EN/RU: Native wait for terminal state, 1 terminal/0 timeout/invalid, one budget 0..120000ms.
 * Does not join or consume input/output. STOPPED may precede completion of provider cleanup.
 * Native-ожидание terminal status, 1/0; не join. STOPPED может предшествовать cleanup provider.
 */
int styxbidirmovieserverwait(StyxBidirMovieServer *server, unsigned timeout);


/* EN/RU: Sticky nonwaiting native cancel; wake RX/TX waits without stopping the shared route.
 * Provider callbacks receive a native scope, must bound/honour their own I/O; never preempted.
 * Необратимая native-отмена без остановки route; I/O provider обязан ограничивать самостоятельно.
 */
void styxbidirmovieserverstop(StyxBidirMovieServer *server);


/* EN/RU: After external callers retire, stop/join owned worker and consume server, 1/0 join failure.
 * NULL succeeds; self-join refuses without freeing. Shared route/TX copies stay with host.
 * После external callers остановить/join свой worker, освободить server; 1/0, NULL — успех.
 */
int styxbidirmovieserverfree(StyxBidirMovieServer *server);
#ifdef __cplusplus
}
#endif
#endif
