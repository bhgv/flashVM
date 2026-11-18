/* Copyright (c) 2026 bhgv
 * EN: Autonomous native publication of existing resource offers on a granted Unibus S-record route.
 * RU: Автономная native-публикация существующих resource offers через выданный Unibus S-route.
 */
#ifndef STYX_BIDIR_MOVIE_OUTGOING_H
#define STYX_BIDIR_MOVIE_OUTGOING_H
#include "route/types.h"
#include "launch/movie_offer.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct StyxBidirMoviePublisher StyxBidirMoviePublisher; /* Owned native worker/source subscription, borrowed route. */
#define STYX_MOVIE_PUBLISHER_READING 0 /* Native source observation and TX publication active. */
#define STYX_MOVIE_PUBLISHER_DONE 1 /* Source CLOSED pair copied completely into shared TX. */
#define STYX_MOVIE_PUBLISHER_STOPPED 2 /* Explicit local cancellation, no source/global grant withdrawal. */
#define STYX_MOVIE_PUBLISHER_FAILED 3 /* Native source, framing, TX, deadline or exact-client failure. */


/* EN: Create owned publisher/NULL on borrowed route, trusted source and separately reserved TX tag.
 * Host MUST negotiate the existing outgoing tag-zero binding-declaration profile and reserve an
 * unused nonzero uint32 tag. RX bindings confer no TX authority. Retain source independently;
 * borrow route through final free. Limit >= MOVIE_OFFER_FRAME+32, timeout 1..120000ms per queued
 * declaration/offer pair. Source may be unpublished initially. One native worker, no guest work.
 * RU: Создать свой publisher/NULL, независимо удержать source. Host согласует TX profile/tag;
 * RX не выдаёт TX-прав. Один native worker, bounded deadline пары, без guest work.
 */
StyxBidirMoviePublisher *styxbidirmoviepublisher(StyxBidirRoute *route, DVM_MovieOffer *source,
    unsigned long tx_tag, unsigned timeout);


/* EN/RU: Copy phase/-1 invalid; optional last fully queued generation (0 none), no I/O.
 * Snapshot is queue admission, not peer receipt or access authority. / Копия phase/generation,
 * -1 invalid, без I/O; это queue admission, не peer receipt и не права доступа.
 */
int styxbidirmoviepublisherstate(StyxBidirMoviePublisher *publisher, unsigned long long *generation);


/* EN/RU: Native wait, 1 minimum generation queued/0 timeout/-1 invalid or terminal before minimum.
 * minimum 0 waits for any terminal phase; timeout 0..120000ms, original monotonic budget retained.
 * Native-ожидание: 1 queued minimum/0 timeout/-1 invalid либо terminal раньше; minimum0 — terminal.
 */
int styxbidirmoviepublisherwait(StyxBidirMoviePublisher *publisher, unsigned long long minimum, unsigned timeout);


/* EN/RU: Sticky local cancellation, no route/client/source shutdown. To withdraw source authority,
 * close the actual source and allow CLOSED publication; stop alone is not a wire withdrawal.
 * Локальная отмена без закрытия source/route/client. Для отзыва source использовать его close.
 */
void styxbidirmoviepublisherstop(StyxBidirMoviePublisher *publisher);


/* EN/RU: After external callers retire, stop/join only its worker, release native pins, 1/0 join failure.
 * NULL succeeds; accepted TX copies remain with host route. / После callers освободить, NULL — успех.
 */
int styxbidirmoviepublisherfree(StyxBidirMoviePublisher *publisher);
#ifdef __cplusplus
}
#endif
#endif
