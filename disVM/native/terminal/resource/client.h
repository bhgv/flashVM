/* Copyright (c) 2026 bhgv
 * EN: One-shot managed resource exchange on a granted typed bidirectional Unibus route.
 * RU: Одноразовый обмен ресурсом по выданному типизированному bidir-маршруту Unibus.
 */
#ifndef STYX_BIDIR_MOVIE_CLIENT_H
#define STYX_BIDIR_MOVIE_CLIENT_H
#include "route/types.h"
#include "managed_source.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct StyxBidirMovieClient StyxBidirMovieClient; /* Owned native one-exchange authority, no route ownership. */


/* EN: Borrow route through all client calls/free, return owned client/NULL without I/O.
 * index must be an exclusively assigned MOVIE_TYPE/MOVIE_VERSION RX queue; tx_tag is a nonzero
 * independently granted outgoing tag of that same schema, never derived from RX declarations.
 * Requires record limit >= MOVIE_FRAME+32 and timeout 1..120000ms. Host services all other queues.
 * One valid fetch attempt consumes this client, including failure; no concurrent/replayed fetch.
 * Retire an incomplete logical resource channel before assigning its queue to another request.
 * RU: Заимствовать route до free, вернуть свой client/NULL без I/O. RX-очередь назначена этому
 * обмену эксклюзивно; TX tag согласован отдельно. Один fetch, включая отказ, без повторов.
 */
StyxBidirMovieClient *styxbidirmovieclient(StyxBidirRoute *route, unsigned index,
    unsigned long tx_tag, unsigned timeout);


/* EN: Use as FVM_DesktopManagedFetch with borrowed client userdata; synchronous native owner only.
 * Encode the existing purpose/reference/base request once and receive one-record replies using a
 * single monotonic budget. No automatic binding announcement, dial, VFS lookup or local fallback.
 * Sink stages bytes on this caller; it must enforce total quota and its own bounded/cancellable I/O.
 * Budget/cancellation checked before/after sink calls, never preempting a callback. Origin publishes
 * only after successful END and final admission; partial/failed results preserve caller output.
 * Native stop/current cancellation scope interrupts a queue wait, leaving other routes operational.
 * Caller must discard staging on non-OK. Invalid arguments refuse before consuming the client.
 * RU: Callback managed fetch, userdata — client. Один запрос, единый monotonic budget, без dial/
 * fallback. Sink выполняется на caller и сохраняет staging; при отказе caller его отбрасывает.
 * Отмена прерывает native queue wait, но не останавливает route или guest.
 */
FVM_Result styxbidirmoviefetch(unsigned purpose, const char *reference, const char *base,
    FVM_DesktopWrite write, void *state, char *resolved, size_t capacity, void *userdata);


/* EN/RU: Sticky nonwaiting native cancellation, NULL harmless; no shared route stop or wire command.
 * Необратимая native-отмена без ожидания, NULL допустим; общий route не останавливается.
 */
void styxbidirmovieclientstop(StyxBidirMovieClient *client);


/* EN/RU: Consume after every fetch/stop caller retires; request stop, release native state, NULL harmless.
 * Освободить после завершения всех fetch/stop callers, NULL допустим; route остаётся у host.
 */
void styxbidirmovieclientfree(StyxBidirMovieClient *client);
#ifdef __cplusplus
}
#endif
#endif
