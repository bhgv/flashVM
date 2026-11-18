# Announced resource channels / Объявленные каналы ресурсов

[`styxbidirmoviereceiver`](announced.h) automatically consumes the **existing**
`flashvm.movie.offer` version 1 announcements from a typed Unibus queue. Resource
fetches then use ordinary `Twalk/Topen` on `movie/<generation>` of the **same 9P
client**. Each open has independent existing S-record request/reply state.
Repeated and parallel loads therefore get separate logical streams through
standard fids, without a new channel-negotiation schema, listener or dial.

The [automatic outgoing producer](OUTGOING.md) connects an existing offer source
to shared Unibus TX. The existing [`styxmovieofferroot`](../../movie/offer.h)
projects its authorized per-job namespace. The host associates
that export with this route's exact granted connection. Ready/closed generation
notices carry no host path, address, user credentials or native capability pointer.

RU: Фоновый native-приёмник читает существующие объявления offer из typed-очереди.
Каждая загрузка открывает отдельный обычный fid `movie/<generation>` того же
соединения. Повторные и параллельные запросы получают независимые S-streams без
нового протокола согласования. Namespace и доступ остаются у исходного VFS service.

## Setup / Настройка

Register one exclusive RX queue with `MOVIE_OFFER_TYPE/MOVIE_OFFER_VERSION` when
creating the route. UI/general and other typed queues remain with their own
native consumers. The constructor obtains and retains the exact client from the
route; it accepts no substitute client, path authority or network locator.

```c
StyxRouteType type = {MOVIE_OFFER_TYPE, MOVIE_OFFER_VERSION};
/* Include type when constructing the route; the selected queue is offer_index. */
StyxBidirMovieReceiver *receiver = styxbidirmoviereceiver(route, offer_index, 5000);
/* Check receiver before installing styxbidirannouncedfetch as the managed callback. */
```

The receiver owns one native offer worker, an existing movie session and a native
terminal subscription. It borrows the route through final free. The worker
accepts one copied offer body per iteration and never becomes another reader of
the raw control stream. No guest polling loop, heap inspection or forced guest
suspension is involved. Fetch calls may wait for their own ordinary native I/O.

## Fetch / Загрузка

[`styxbidirannouncedfetch`](announced.h) has the existing managed-fetch callback
signature. Set its userdata to the receiver. Callers provide independent native
staging and output storage; commit staging only on `FVM_OK`.

```c
FVM_Result result = styxbidirannouncedfetch(FVM_DESKTOP_FETCH_CODE,
    "/app/main.swf", "/app/start.swf", stage, staging_state,
    resolved, sizeof(resolved), receiver);
/* Other native consumers continue servicing UI/general. */
/* Commit staging on FVM_OK; discard it on every other result. */
```

The callback validates arguments before waiting. Its configured readiness budget
is `1..120000` milliseconds. Once ready, it captures a positive generation, opens
one fresh service fid and uses the original resource request/reply codec. The
existing Styx RPC bounds govern transfer; this change adds no total-file deadline.
CODE/DATA intent retains the service's executable/provider-first or ordinary
namespace lookup policy. No local-file or alternate-client fallback is added.

Repeated calls after success, provider refusal or sink refusal open independent
streams. Parallel callers must use separate sink/output state and keep the
receiver alive. A failed acquisition's fid is retired normally; its partial
response cannot become another fid's data. Limits on outstanding native RPCs and
staging size remain the caller/client configuration, not an unbounded task pool.

The selected generation is checked before walk and after resource completion.
Typed offer acceptance and final origin publication share a short native mutex.
A changed/closed offer or stopped route causes staged data to be rejected; the
caller-visible origin remains unchanged. No accepted operation is replayed.

RU: Один callback — один новый fid и собственные staging/output. При смене
generation, закрытии или отказе частичные bytes отбрасываются caller; origin не
публикуется. Повторная загрузка использует новый stream. Общий кодек и правила
VFS те же, что у обычного Styx-loader; UI/general не читаются этим callback.

## Readiness and lifetime / Готовность и время жизни

| API | Result |
| --- | --- |
| `styxbidirmovieavailable` | Authoritative local availability: 1 ready, 0 pending, -1 retired/invalid; optional copied generation. |
| `styxbidirmoviereceiverwait` | Native wait for generation at least minimum (0 any): 1 ready, 0 timeout, -1 retired/invalid. No record consumption. |
| `styxbidirmoviereceiverstate` | Diagnostic READING/CLOSED/FAILED/STOPPED, -1 invalid. Use availability for admission. |
| `styxbidirmoviereceiverstop` | Retire the private session and wake its native waits; shared route/client stay with host. |
| `styxbidirmoviereceiverfree` | After external callers retire, stop/join only its offer worker and release native pins; 1 success/0 join failure, NULL succeeds. |

The same existing payload validator serves raw Styx control readers and typed
offers. Unknown versions stay in the general route queue. Older notices cannot
replace a newer generation; exact repeats are idempotent. Contradictory notices
for one generation and malformed payloads fail closed locally. CLOSED remains
terminal for that receiver. No incoming declaration is a TX permission.

Native connection retirement wakes the worker through a separately pinned
cancellation source. Watch callbacks reference that source rather than a receiver
pointer. [Fetch-local cancellation](FETCH-CANCEL.md) now forwards receiver retirement and
outer-caller cancellation through independent per-call sources. Ordinary resource
RPCs use the existing fresh-token/Tflush binding; native sinks can observe the
current scope. No arbitrary callback is forcibly preempted. Final session admission
still rejects changed/closed generations. The host must retire **all** external fetch/state callers
before free, and keep the borrowed route alive until free completes.

RU: Готовность обновляется автоматически. Старые объявления не откатывают новое
generation; противоречивые и повреждённые объявления завершают только receiver.
Stop/free не закрывают общий route/client и не ждут участия guest. Перед free
host завершает все внешние fetch/state calls; native RPC отменяется через прежний Tflush,
а sink соблюдает собственный контракт native I/O без принудительного прерывания.

## Verified scope / Границы проверки

The [original 9P fixture](../tests/offers_client.c) uses native bidir peers over
one fragmented PTY with ordinary `movie/1`, `movie/2`, `movie/3` nodes. Each open
is backed by the actual VFS resource service. It checks repeated and three-way
parallel fetches, delayed readiness, generation change/CLOSED during I/O, sink
refusal and recovery, unsupported versions, malformed/conflicting/stale notices,
constructor failure, explicit stop, route loss and provider refusal. UI/general
records and ordinary session operations remain independently usable.

The fixture's generation files are explicit test grants; production exports use
the scoped offer-root API. Fixture binary bytes are not executed SWF content.
Actual FlashVM loading remains in the existing managed-player tests. Automatic
general bulk/video/audio channel negotiation and `file/state` control nodes remain
separate work; resource stream isolation here uses existing ordinary fids.
