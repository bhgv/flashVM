# Automatic offer publication / Автоматическая публикация объявлений

[`styxbidirmoviepublisher`](outgoing.h) connects a trusted native `DVM_MovieOffer`
source to the existing Unibus TX queue. One native worker observes its independent
pollable source outbox. Initial publication, replacement and source retirement
produce the existing `flashvm.movie.offer` version 1 `READY/CLOSED + generation`
records. No Limbo polling, guest callback or guest suspension is required.

RU: Источник автоматически сообщает о готовности, смене поколения и отзыве
ресурса. Producer сам подписывается на изменения; Limbo может работать независимо.
Все объявления передаются в S-записях через общий транспорт Unibus.

## Host setup / Настройка host

Before construction the host must explicitly agree the existing **outgoing
binding-declaration profile** and reserve an unused nonzero uint32 TX tag. An
incoming declaration never supplies TX permission. The tag is not fixed by this
API. Hosts also associate the source's scoped
[`styxmovieofferroot`](../../movie/offer.h) export with the exact granted 9P
connection. An offer contains no root pointer, host path, credential or address.

```c
/* Existing grant/export setup and independently reserved offer_tx_tag precede this. */
StyxBidirMoviePublisher *publisher = styxbidirmoviepublisher(
    route, source, offer_tx_tag, 5000);
/* Check publisher; keep route alive through publisherfree. */
```

The constructor returns an owned handle or NULL. It independently retains the
source through a private existing `styxmovieofferwatch` outbox, borrows the route
until final free, and requires a record limit of at least `MOVIE_OFFER_FRAME+32`.
The `1..120000` millisecond timeout bounds admission of each declaration/offer
pair, rather than the lifetime of the source. An initially empty source is valid;
its first bind wakes the native worker automatically.

Each publisher has a separate outbox and reserved TX tag. Several connections or
independent subscribers can observe the same source. The host still prepares and
grants each exported namespace separately; sharing the notification source does
not bypass export access checks or authenticate a remote user.

## Publication and backpressure / Публикация и очереди

The existing outbox snapshots the latest coherent source state when read. Rapid
changes can coalesce; intermediate generations can be skipped and identical
notices can repeat. Once captured, one declaration/offer pair remains immutable
until queue admission succeeds or fails. This is latest-state notification, not
a lossless history of every source transition.

The worker copies the complete declaration S-record, then the complete offer
S-record into bounded shared TX. Both use one monotonic admission deadline.
Other producers may insert **whole records** between them; bytes within records
never interleave. Hosts must preserve the reserved tag's binding association.
There is no raw-stream writer and no automatic replay of an accepted frame or an
uncertain transport prefix. A failed pair can leave a valid declaration queued
without its offer; the publisher's generation advances only for a complete pair.

Idle observation waits on the native outbox descriptor. Full TX waits on native
capacity notification, with cancellation. Neither wait pauses a guest thread.
The host continues servicing UI, general and unrelated typed queues; an unserved
full route queue can delay later traffic. Separate logical channels remain the
way to isolate latency-sensitive traffic.

RU: Очередь ограничена. Ожидание места имеет единый срок для пары S-записей;
между целыми записями возможен другой трафик, между их байтами — нет. Принятые
записи не повторяются. Частые изменения объединяются до актуального состояния;
уже взятая пара остаётся неизменной.

## State and lifetime / Состояние и время жизни

| API | Meaning |
| --- | --- |
| `styxbidirmoviepublisherstate` | Coherent phase and optional last fully queued generation; generation 0 means none. -1 invalid. |
| `styxbidirmoviepublisherwait` | One native monotonic wait: 1 minimum generation queued, 0 timeout, -1 invalid or terminal before minimum. Minimum 0 waits for any terminal phase. |
| `styxbidirmoviepublisherstop` | Sticky local STOPPED, wake source/TX waits; shared source, route and client remain owned by their hosts. |
| `styxbidirmoviepublisherfree` | After all external callers retire, stop/join only this worker and release private native pins. 1 success, 0 join failure; NULL succeeds. |

READING means active observation; DONE means a CLOSED pair was copied completely
into TX; FAILED means source/framing/admission/transport failure. These phases
and the queued generation do **not** promise peer receipt, execution or access.
Use the existing flush for native write acknowledgement when needed; that is
still not an application-level acknowledgement.

To withdraw a resource, close the actual source, allow CLOSED publication and
keep the route alive through the required delivery/teardown policy:

```c
if (dvm_movie_offer_close(source, expected_generation))
{
    int ended = styxbidirmoviepublisherwait(publisher, 0, 5000);
    /* Inspect ended and phase; queued CLOSED alone does not prove peer receipt. */
}
/* Retire external publisher callers before final free. */
styxbidirmoviepublisherfree(publisher);
```

Stopping or freeing one publisher alone is **not** a wire withdrawal and does not
close the shared source. Other subscribers continue observing it. Already accepted
TX copies remain route-owned after publisher disposal. Native underlying-client
or local-route retirement wakes the publisher, including an idle source wait.
Callbacks own independent cancellation/outbox pins and never dereference the
publisher or route after disposal.

[`styxbidircontrolwatch`](../control.h) and
[`styxbidirroutewatch`](../route/route.h) expose sticky native local retirement
subscriptions. They return owned `DVM_CancelTap` handles; remove them with
`dvm_cancel_unsubscribe`. Callback state is adopted only on success and released
after admitted callbacks retire. A late subscription replays retirement; callbacks
may run before construction returns and may repeat. Keep callbacks native,
idempotent and nonwaiting, without VM/provider/transport I/O. Subscription lifetime
can outlive the disposed transport because it pins only its notification source.

RU: Stop/free producer прекращает только его подписку. Для настоящего отзыва
закрывается сам source и отправляется CLOSED. Общий клиент остаётся открытым.
Нативные подписки на завершение route будят ожидания даже после ошибки TX;
их callback-state может безопасно завершиться позже самого producer.

## Verification and limits / Проверка и границы

The [original 9P fixture](../tests/publisher_host.c) checks initial/delayed READY,
replacement, initially CLOSED, independent subscribers, coalescing under gated
TX, idle stop, cancelled/expired TX admission, uncertain write and constructor
failure. UI-enabled and data-only routes use the same mechanism. A fixture-only
peer echoes checked complete frames; the automatic incoming receiver then fetches
real binary resources using independent ordinary `movie/<generation>` fids of
that same client. These generation files are explicit fixture grants, not proof
of production authorization by a guessed name. No fixture binary is executed SWF.

Additional checks cover canonical/invalid/native64 payloads, C++98 linkage and
sticky notification subscriptions before/after route disposal. Production
source-root authorization remains in the existing offer-root/VFS API.

This adapter uses the existing scoped resource protocol. General bulk/media
channel negotiation, `file/state` control-node projection, mixed Git revisions,
module migration and a complete remote IDE launch remain separate work.
