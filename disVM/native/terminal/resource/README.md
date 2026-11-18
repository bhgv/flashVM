# Resource exchange on Unibus / Обмен ресурсом через Unibus

[Announced per-open channels / Объявленные per-open каналы](ANNOUNCED.md) now
support repeated/parallel fetches through ordinary fids on the same exact client,
with automatic typed offer reception and generation fencing.

[Autonomous VFS server / Автономный VFS-сервер](SERVER.md) supplies the native
provider-side handler for an already granted resource channel.

`styx_terminal_resource` supplies a native managed-fetch callback for one existing
`flashvm.movie` version 1 request/reply exchange on a granted bidirectional
S-record route. It uses the same request codec, VFS-side request interpretation
and [reply receiver](../../movie/REPLIES.md) as the ordinary Styx loader.
There are no new S-tags, request opcodes, 9P messages or physical carriers.

RU: Native-клиент передаёт существующий request и принимает reply через typed
S-очередь. Кодек запроса общий с обычным Styx-клиентом и VFS-провайдером.
Формат wire не меняется; сетевые теги назначает согласованный host-протокол.

## Setup and ownership / Настройка и владение

[`styxbidirmovieclient`](client.h) borrows a live route, an exclusively assigned
resource RX queue index and an independently granted outgoing tag. The registered
queue must match `MOVIE_TYPE` and `MOVIE_VERSION`. An RX declaration is not an
outgoing grant. Record limit must be at least `MOVIE_FRAME + 32`; timeout is
`1..120000` milliseconds. Constructor validation/allocation performs no transport
I/O. The host keeps the route alive through all client users and final free.

This client is **one-shot**. One valid fetch attempt consumes it, whether that
attempt completes, times out, is refused by TX admission or fails at the provider.
Another concurrent/later call cannot send another request or consume its replies.
Invalid arguments refuse before one-use admission. Explicit stop is sticky even
before admission. Free only after all fetch/stop callers retire.

The host assigns a fresh logical resource channel, checks peer authority and
services UI/general/other queues independently. The client refuses an already
nonempty resource RX queue before sending; that check is not a substitute for
channel lifecycle. After an incomplete exchange, retire that logical channel
before assigning its queue to a new request. No correlation ID exists in this
schema, so late replies cannot safely be relabelled for a later exchange.

RU: Один клиент — одна допустимая попытка fetch, включая отказ TX или timeout.
Нельзя повторно использовать его для запоздавшего ответа. При незавершённом
обмене host завершает логический resource channel перед следующим запросом.
Предварительно заполненная очередь отклоняется без потребления; это не заменяет
правильный lifetime канала. Общий UI/general route из-за этого не закрывается.

## Managed-fetch callback / Callback загрузки

Use [`styxbidirmoviefetch`](client.h) with this client as the managed player's
userdata, or call it from the appropriate native adapter:

```c
StyxBidirMovieClient *fetch = styxbidirmovieclient(route, resource_index,
    granted_tx_tag, 5000);
/* Check fetch before use. Another native owner services other route queues. */
FVM_Result result = styxbidirmoviefetch(FVM_DESKTOP_FETCH_CODE,
    "/app/main.swf", "/app/start.swf", stage, staging_state,
    resolved, sizeof(resolved), fetch);
/* Commit staging only on FVM_OK; discard it otherwise. */
styxbidirmovieclientfree(fetch);
```

CODE/DATA is explicit intent, never guessed from a suffix. The provider's
existing VFS service resolves the request with its configured code/data namespace
policy and makes the access decision. Reference and base retain their existing
counted-byte layout. The native client performs no local file fallback, dial,
host-path translation, implicit TX binding announcement or VFS lookup.

The callback runs synchronously on its caller's native owner. No hidden guest
executor, foreign heap access or cooperative polling loop is introduced. Only
valid BYTES invokes the staging sink on that same owner. The caller supplies
total quotas and rollback. The returned path is metadata, not authority.

The client attempts one bounded TX enqueue; a full/terminal queue returns error
without automatic retry. Queue acceptance alone is not success. Only the
existing successful END, within the live budget and final admission, publishes
the origin to the caller. All intermediate origin text is private.

## Deadline and cancellation / Deadline и отмена

One monotonic budget covers enqueue and all reply waits; chunks do not renew it.
The budget and cancellation are checked before/after synchronous sink calls.
The client cannot preempt a native callback: sinks must bound their own I/O and
honour the propagated native cancellation scope when applicable. Late completion
after budget expiry is not reported as successful acquisition.

`styxbidirmovieclientstop` wakes an active native queue wait through an independent
cancellation source. The caller's existing `DVM_CancelScope`, if present, forwards
into that source too. Forwarded callbacks pin the source itself rather than a
client/route pointer. The callback restores the caller's original scope on return.

The [typed route API](../route/types.h) now provides
`styxbidirroutetypedmatches` for immutable registration checks and
`styxbidirroutetypedwaitcancel` for cancelable native readiness. Cancellation does
not consume a record, stop the route or invalidate another waiter. Its private
callback context detaches route access before return, including unsubscribe races.

RU: Deadline один на весь обмен, не новый на каждый чанк. Sink работает на caller;
принудительно прерывать callback нельзя, собственные native I/O он ограничивает
сам. Отмена будит native wait без остановки route, VM или других потребителей.
Источник внешнего native scope передаётся без хранения client/route в callbacks.

## Verified scope / Границы проверки

[`exchange_client.c`](../tests/exchange_client.c),
[`exchange_server.c`](../tests/exchange_server.c) and
[`exchange_provider.c`](../tests/exchange_provider.c) exercise both original 9P
sessions over one fragmented PTY. A test-only dispatcher forwards the received
request to the real namespace-backed VFS resource service; its reply bodies are
S-framed beside UI and opaque records. The source contains independent binary
bytes, not an executable SWF fixture.

Cases cover code/data intent, provider refusal, stop before/after submission,
timeout, preserved sink refusal, a deterministic concurrent contender, outer
scope cancellation, deterministic TX admission refusal, prequeued stale replies,
late replies and native cancellation/subscription races. The ordinary session
remains usable after resource client retirement. Existing movie-player acceptance
tests separately exercise actual FlashVM loading.

The generic [provider-side worker](SERVER.md) now executes requests through the
actual VFS service. Automatic channel allocation/correlation and `file/state`
VFS control nodes remain separate work; this is not a complete remote application launch.

The one-shot handle now [forwards route retirement into its active native sink scope](FETCH-CANCEL.md),
not only queue waits. Its retirement callback retains only a cancellation source,
so late delivery cannot dereference a freed client handle. Announced per-call
isolation and ordinary resource RPC cancellation are described in the same guide.
