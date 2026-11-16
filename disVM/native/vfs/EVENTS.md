# Native provider event resource / Native ресурс событий провайдера

Runtime0.150 provides an ordinary readonly VFS file backed by an automatically
subscribed provider change journal. One resource owns one bounded publication
inbox. A native producer publishes a normal journal event; projection and
readiness happen immediately, without a Limbo loop, guest execution, scheduler
poll or guest suspension. The embedding host supplies explicit access filtering
and record encoding. No default public wire format or global path is installed.

## Construction and ownership / Создание и владение

`styxeventsource(journal, &policy)` retains a borrowed live journal independently
of its provider and returns an owned `StyxEvents*` or NULL. Only success adopts
`policy.state`. All descriptor/memory setup precedes subscription. Subscription
followed by a current-state snapshot closes the missed-change registration race;
source session/generation fencing drops stale concurrent callback deliveries.

`StyxEventPolicy` requires both `allow` and `encode` callbacks, a nonzero subset
of the six existing change classes and an encoded capacity of1..65536 bytes.
`resources=0` hides resource identity/flags before formatting;1 explicitly exports
them. Only an exact1 admits/encodes; invalid or negative callback results fail closed.
`allow` receives private source scope for filtering and runs both on native
publishers and immediately before read delivery, including cached heads. Rejecting
a queued head discards it without exposing its bytes. An already admitted read can
finish; external policy changes are not atomically coupled to copying bytes.

Policy callbacks must be native, bounded, thread-safe and must not reenter their
inbox, perform provider/network I/O or execute guests. Keep the codec/context
immutable; synchronize any changing access-policy state explicitly. The producer
never formats records. The serialized native read caller invokes `encode`, which
must return1 and a nonzero in-capacity byte count. Failure/malformed length keeps
the head for retry. `release(state)` runs exactly once after every owner/read/tap
callback has retired. Failed construction leaves state caller-owned.

`styxeventsretain/free` manage native owner references. Final owner release
unsubscribes without waiting for publishers; an already admitted callback pins
the resource and policy until return. The resource retains only its journal,
not a provider/Prog/guest reference, avoiding a provider subscription cycle.
`styxeventsfinish` detaches the source explicitly and preserves queued records
for drain, then EOF. It does not infer termination from a CONNECTION-class hint:
that hint might describe temporary suspicion or recovery. Journal-owner release
alone does not finish a resource that independently retains that journal.

```c
/* Native host chooses a journal already scoped/authorized for this publication. */
StyxEvents *events = styxeventsource(journal, &policy);
/* Retain a separate native owner while its readiness fd is registered. */
if (events && styxeventsretain(events)) {
    DVM_Provider *file = styxeventprovider(events); /* Consumes retained ref always. */
    /* Bind/publish file in the prepared authorized tree using existing APIs. */
    /* Caller still owns events; file/provider handles own their own lifetime. */
}
```

`styxeventprovider` consumes one owned inbox reference on every outcome. Its root
is an ordinary stable-identity readonly file; only dot/r opens succeed. Metadata
basename is `events`, but the prepared namespace can bind/mask it under another
name. No automatic namespace authority, PID selection or nsadmin role is added.
Existing aliases and opens share the same queue/head. Construct separate sources
for independent subscribers, prepared connection roots or different permissions.

## Security and projection / Доступ и проекция

The host must select an authorized journal and filter before publication.
`allow` is mandatory: a publication cannot accidentally default to exporting all
metadata. Resource IDs are opt-in, source names/paths/credentials/native pointers
are never part of the scalar event. Delivery rechecks use retained private scope
even when public resource IDs are hidden. This prevents cached metadata surviving
an access revocation that occurred before read admission.

Gaps, epoch changes and queue overflow conservatively broaden to the selected
class mask with provider-wide scope. The filter must authorize such view-wide
invalidation hints as well as individual objects. This is a refresh hint about
the granted view, not a grant to inspect a hidden object. Source generations,
timing and broad change hints can still reveal activity: use an independently
scoped journal when those observations are confidential. Publication/root
selection and ordinary VFS permissions remain the embedding host's responsibility.

## Reads and bounds / Чтение и ограничения

The queue holds32 tail events plus at most one retained head. Overflow coalesces
only the tail into a broad refresh hint; it never overwrites a head already held
for formatting/short-buffer retry. There is one fixed-capacity encoding buffer.
No per-event allocation occurs on the producer path. Only changed source reports
enter the queue; cached formatting is reused across short reads/close/reopen.

`styxeventsread` consumes exactly one complete record. The required count output
is initialized on every outcome. Size0 consumes nothing. Multiple native readers
serialize through one resource reader mutex; they compete for this shared inbox,
not duplicate its events. A read examines at most33 denied heads, so concurrent
policy rejection/publication cannot create an unbounded scan.

| Scalar result | Ordinary SDK result | Meaning |
|---|---|---|
| `STYX_EVENT_RECORD` | OK + RECORD | One complete record copied/consumed |
| `STYX_EVENT_EMPTY` | AGAIN, not EOF | Live source has no deliverable event |
| `STYX_EVENT_SMALL` | INVALID_ARGUMENT | Buffer too short; head/cache preserved |
| `STYX_EVENT_ERROR` | FAILURE | Encoding/invalid input; no invented EOF |
| `STYX_EVENT_EOF` | OK + EOF | Explicitly finished source completely drained |

Consumers still refresh actual provider state after invalidation. Changes do not
freeze a tree or grant access. Encoding capacity must fit the intended transport's
negotiated record/read limits; an oversized record stays pending for a larger
read or codec/backend policy rather than being truncated. A successful local
read/response attempt does not prove remote receipt; reconnect/replay semantics
remain separate from this queue's lifetime contract.

## Native Styx integration / Native Styx integration

`styxeventsfd` borrows a nonblocking CLOEXEC readiness descriptor while an owner
ref lives. The resource owns/drains/rearms it; callers must not read or close it.
Callbacks only issue nonblocking native readiness signals, never provider/network
I/O. Record reads maintain level readiness, including short/error head retention
and finished EOF. Register it only while reads are waiting, and unregister before
releasing the owner. Otherwise queued data/EOF will correctly remain readable and
can cause a host that keeps polling without reading to spin.

A native socket owner registers the descriptor with `styxnotifyfd` or its own
readiness aggregator. Multiple resource descriptors can be aggregated by the
host; libstyx's existing POSIX notify hook itself accepts one borrowed descriptor.
The ordinary file bridge now exposes `StyxTransfer.again`: rebuild native consumers
with the new scalar layout. An empty resource returns AGAIN, allowing the owner
to call `styxdeferread` and set `*n=0`. When ready, the owner reads the retained
`styxreadaux` object at `styxreadoffset` and completes the original Rread/Rerror.
Only true finished/drained EOF becomes an empty positive-capacity Rread.

[Actual SDK backend](../tests/event-resource/backend.c) and
[original C Styx embedding](../tests/event-resource/wire.c) demonstrate journal
publication from a native producer, readonly provider opens, dynamic readiness
registration, deferred reads, cancellation and teardown. Their text codec and
fixture tree/control names are test data, not public protocol conventions.

## Acceptance / Проверки

The native suite has97 tests. New queue tests cover initial snapshot, explicit
filters, hidden classes/IDs, per-read revocation of cached metadata, short buffers,
formatter failures, cache retention across close/reopen, bounded overflow,
ordinary SDK RECORD/AGAIN/EOF, scalar bridge AGAIN, fd flags/readiness and finish.
Controlled native-thread fixtures verify nonwaiting final-owner teardown while
a callback is in flight, reordered report fencing and shared readers without
duplicate consumption. Real TCP tests verify one-record replies, no hidden
resource/class leakage, deferred short-buffer preservation, reopen, Tflush,
source finish/drain and server end. Actual native client tests combine the real
provider source, automatic heartbeat and explicit cancellation/fresh reads.
ASan/UBSan use `detect_leaks=0`; no LeakSanitizer or physical-platform claim is made.

This provides the configurable server-side event resource. Runtime0.151 adds [configured native remote-to-local feeds](FEED.md). Automatic
subscription discovery, a standard record codec and load-value schemas remain open. Existing local
per-thread system inbox format remains unchanged. Original Inferno libstyx/lib9
sources and 9P messages are unchanged in this step; no graphics are added.

Ресурс — обычный readonly файл в подготовленном дереве. Журнал подписывается один
раз; дальше изменения автоматически попадают в bounded очередь и будят native
socket-owner. Никакого Limbo polling или приостановки гостя. Host явно задаёт
фильтр и кодировщик; публичный wire-формат и глобальное имя не фиксируются.
Один read выдаёт одну запись. Короткий буфер сохраняет запись, переполнение даёт
консервативную инвалидизацию, а EOF появляется после finish и полного drain.
Фильтр проверяется и перед выдачей cached записи; приватный scope хранится для
этой проверки, но скрывается от кодировщика при resources=0. Права, публикацию и
раздельные деревья соединений по-прежнему определяет существующая VFS-политика.
