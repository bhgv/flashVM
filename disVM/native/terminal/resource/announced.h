/* Copyright (c) 2026 bhgv
 * EN: Automatic typed Unibus offer reception with ordinary per-open Styx resource channels.
 * RU: Автоматический приём typed Unibus offers с обычными per-open Styx resource channels.
 */
#ifndef STYX_BIDIR_MOVIE_ANNOUNCED_H
#define STYX_BIDIR_MOVIE_ANNOUNCED_H
#include "route/types.h"
#include "managed_source.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct StyxBidirMovieReceiver StyxBidirMovieReceiver; /* Owned native offer worker/session, borrowed route. */
#define STYX_MOVIE_RECEIVER_READING 0 /* Native offer consumer is active, pending or ready. */
#define STYX_MOVIE_RECEIVER_CLOSED 1 /* Announcement direction or exact client retired. */
#define STYX_MOVIE_RECEIVER_FAILED 2 /* Malformed offer/protocol/native setup failure. */
#define STYX_MOVIE_RECEIVER_STOPPED 3 /* Explicit host cancellation, no shared route stop. */


/* EN: Create owned receiver/NULL for an exclusive MOVIE_OFFER_TYPE/VERSION typed RX queue.
 * Borrow route through final free; obtain/retain the exact client from that route, no independent
 * connection argument, dial, fallback or new wire schema. One native worker consumes one offer
 * per call; UI/general remain with host. ready_timeout 1..120000ms bounds initial fetch readiness.
 * Repeated/parallel fetches use independent ordinary fids on movie/<generation> of this same client.
 * RU: Создать owned receiver/NULL на exclusive offer RX queue, без нового соединения/схемы.
 * Один native worker принимает offers, последующие fetch используют отдельные обычные fids.
 */
StyxBidirMovieReceiver *styxbidirmoviereceiver(StyxBidirRoute *route, unsigned index, unsigned ready_timeout);


/* EN/RU: Current authoritative availability 1 ready/0 pending/-1 retired/invalid; optional generation copy.
 * Native local queries only, no queue consumption or transport I/O. / Текущая availability 1/0/-1,
 * optional generation, без потребления очереди и transport I/O.
 */
int styxbidirmovieavailable(StyxBidirMovieReceiver *receiver, unsigned long long *generation);


/* EN/RU: Native readiness wait for generation>=minimum (0 any), 1 ready/0 timeout/-1 retired/invalid.
 * timeout 0..120000ms, one monotonic deadline; no guest execution or queue consumption.
 * Native-ожидание readiness generation>=minimum, 1/0/-1, без guest execution/потребления очереди.
 */
int styxbidirmoviereceiverwait(StyxBidirMovieReceiver *receiver, unsigned long long minimum, unsigned timeout);


/* EN/RU: Copied READING/CLOSED/FAILED/STOPPED/-1 invalid, no I/O; status is not an access grant.
 * Копия phase либо -1 invalid, без I/O; status не выдаёт прав.
 */
int styxbidirmoviereceiverstate(StyxBidirMovieReceiver *receiver);


/* EN: Managed-fetch callback with borrowed receiver userdata, native caller owns sink/staging.
 * Wait at most configured readiness budget, then walk/open one independent existing service fid.
 * Transfer uses existing Styx RPC bounds; no new total-file deadline is implied. Validate generation
 * before walk and after completion; publish caller origin only while that same offer remains live.
 * Repeated/parallel calls allowed with independent staging/output; host must retire every external
 * caller before free. Non-OK requires staging discard; no automatic replay or local fallback.
 * Each call owns a child scope forwarding receiver/caller cancellation; fresh RPC tokens and scope-aware
 * sinks can wake natively. Caller cancellation never retires the shared receiver or other fetches.
 * RU: Managed fetch с receiver userdata, отдельные fid/child scope на вызов. Проверить generation до/после
 * I/O, публиковать origin только при живом offer. Scope-aware sink и fresh RPC tokens учитывают отмену,
 * без обратной отмены receiver/других calls. Staging принадлежит caller, без replay/fallback.
 */
FVM_Result styxbidirannouncedfetch(unsigned purpose, const char *reference, const char *base,
    FVM_DesktopWrite write, void *state, char *resolved, size_t capacity, void *userdata);


/* EN/RU: Sticky native stop and session invalidation, no shared route/client shutdown; NULL harmless.
 * Native stop/инвалидация session без закрытия общего route/client, NULL допустим.
 */
void styxbidirmoviereceiverstop(StyxBidirMovieReceiver *receiver);


/* EN/RU: Consume after external fetch/state callers retire; stop/join only owned worker, 1/0 join failure.
 * NULL succeeds; never self-join. / Освободить после external callers, join только своего worker, 1/0.
 */
int styxbidirmoviereceiverfree(StyxBidirMovieReceiver *receiver);
#ifdef __cplusplus
}
#endif
#endif
