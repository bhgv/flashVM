# Independent native subscriptions / Независимые native-подписки

Runtime0.153 adds a bounded native subscription factory over an authorized
provider journal. Every successful `styxsubscribe` creates a distinct event inbox,
source cursor, bounded queue, encoded-head cache and readiness descriptor. A read
consumes one event from that subscriber only. A slow subscriber's overflow,
short read, cancellation or close does not consume another subscriber's events.
Projection still runs automatically on native journal publications, without a
Limbo loop, guest cooperation, scheduler polling or guest suspension.

This is an extension of the native embedding API. Original Inferno Styx/9P
messages and lib9/libstyx codecs are unchanged. No public event encoding, global
path, discovery protocol or transport reconnect policy is selected.

## API and ownership / API и владение

Include `subscriptions.h`. The host explicitly chooses a journal and immutable,
thread-safe `StyxEventPolicy`, using the same access/encoding rules as
[the event resource](EVENTS.md). Different authorization domains or confidential
views should use distinct scoped journals/policies/factories.

| Operation | Result and contract |
|---|---|
| `styxsubscriptions(journal, &policy, limit)` | Owned factory/NULL; retain journal, adopt policy.state only on success |
| `styxsubscribe(factory)` | Owned independent `StyxEvents`/NULL; no capacity/event wait |
| `styxsubscriptionsinfo(factory, &info)` | Copy limit, active, closed;1/0 |
| `styxsubscriptionsclose(factory)` | Permanently close new admission; existing streams continue |
| `styxsubscriptionsretain(factory)` | Retain borrowed owner;1/0 |
| `styxsubscriptionsfree(factory)` | Consume owner; last owner closes admission nonwaiting |

Every caller pins a factory owner across its call. Limit is1..1024 subscriptions.
A slot is reserved before allocation/subscription, so simultaneous opens cannot
exceed it. Active includes reserved setup, live inboxes and inbox callbacks that
are still retiring. Allocation/descriptor failure rolls back that slot. An owned
finished inbox still holds its slot and policy; release it to admit another one.
There is no waiting queue, eviction or implicit retry when quota is exhausted.
OS fd/memory limits can refuse setup before the configured limit is reached.

Factory construction creates no subscription or readiness descriptor and invokes
no allow/encode callback. Each subscription uses the existing source registration
followed by a current-state snapshot, with source-generation fencing. A new
subscriber gets a refresh hint for its current authorized state; it does not
replay old records or inherit another reader's cursor/cache.

```c
/* Native host owns policy.state until successful factory construction. */
StyxSubscriptions *factory = styxsubscriptions(journal, &policy, 32);
StyxEvents *events = factory ? styxsubscribe(factory) : NULL;
if (events && styxeventsretain(events))
{
    /* Provider consumes this extra source ref; retain events for readiness/finish. */
    DVM_Provider *file = styxeventprovider(events);
    /* Bind/publish file in this connection's prepared authorized tree. */
    /* Register styxeventsfd(events) while this connection has a pending read. */
    /* Bind/open operations retain their own owners; drop this local provider owner. */
    dvm_provider_release(file);
}
/* Finish/free each owned events when that subscription is no longer needed. */
styxeventsfree(events);
styxsubscriptionsfree(factory);
```

A native server can instead call `styxsubscribe` on each authorized Topen, wrap
that source with `styxeventprovider`, and retain its fd through the fid context.
Prepared publication policy still decides what clients may open. Reusing one
returned `StyxEvents`/provider for several opens deliberately shares that one
queue; independent delivery requires separate successful subscriptions.

## Policy, close and lifetime / Policy, закрытие и lifetime

Allow is mandatory and rechecks delivery, including each stream's cached head.
Resource IDs remain opt-in. Generation gaps and per-stream overflow broaden to
the configured class mask; notifications remain refresh hints, never permission
grants. The ordinary operation and prepared namespace still decide access.

Allow/encode receive the factory's shared policy.state, outside factory locks.
Allow may run on simultaneous native publishers/readers; different subscribers
may encode concurrently. There is no global encoder lock. Callbacks must be
bounded, thread-safe, native-only and perform no guest/provider/network I/O or
reentry into their inbox/factory. Synchronize dynamic authorization state in the
host policy. Use separate factories for codecs needing distinct mutable contexts.

Close linearizes new admission under the factory mutex. Setup admitted before
close can finish and return a valid source afterward. Close does not revoke old
streams or discard their queued events. For revocation, update actual access
policy; for one stream's orderly EOF, use `styxeventsfinish` and drain it. Existing
subscriptions continue after explicit close or final factory-owner release.

Every source holds a private factory reference until all its source owners and
already admitted callbacks retire. The factory has no list of source/provider
pointers and cannot form a provider subscription cycle. Its journal/context pins
remain valid while any source needs them. Final policy.release runs exactly once,
outside locks, after every owner, setup and subscriber callback retires. It must
be nonwaiting. Factory close/free never joins a publisher or subscriber.

## Native Styx server example / Пример native Styx server

[Fixture backend](../tests/event-resource/backend.c) demonstrates shared and
per-open independent sources behind the same ordinary readonly provider API.
[Original C Styx embedding](../tests/event-resource/wire.c) retains each source
in its fid context and aggregates pending sources' readiness with a native Linux
epoll fd passed to the existing `styxnotifyfd` hook. Readiness is refreshed after
record reads; completed/flushed/clunked reads unregister their source before fid
cleanup. A shared descriptor remains registered while another pending read uses
it. Registration is limited to pending reads, avoiding queued/EOF readiness spins.

This aggregator is test/example host code, not a new libstyx protocol or a core
library dependency on Linux epoll. The factory itself uses the existing native
source primitives. Other embeddings may aggregate their borrowed descriptors
with their own supported native readiness mechanism.

## Acceptance / Проверки

The native suite has132 tests. Independent-source tests cover descriptor/cache
separation, cached-head access revocation, overflow isolated from a fast reader,
slot reuse, factory close with live readers, empty factory lifetime and failed
fd allocation rollback. Native races cover close during admitted setup, publisher
retirement retaining quota/context, and sixteen simultaneous callers competing
for four slots. Test-only barriers hold native callbacks to expose lifetime;
no guest progress is required.

Actual TCP tests use the original C libstyx server/codecs. Two clients each
receive the same journal changes; a third subscription is refused at the limit
and succeeds after a fid is clunked. Short buffer, Tflush and clunk/reopen on one
subscriber leave the other intact. Hidden resource changes stay filtered and
gaps cause conservative refresh. Two actual native mounted feeds independently
route one publication into their mount journals and continue ordinary RPCs.
The full native suite also runs under ASan/UBSan with `detect_leaks=0`.

Одна native-фабрика задаёт авторизованный journal, общую policy и лимит. Каждый
`styxsubscribe` создаёт отдельную очередь: подписчики больше не забирают события
друг у друга. Лимит учитывает setup и завершающиеся callbacks. Закрытие factory
запрещает новые подписки, существующие продолжают работать до своего finish/free.
Формат/путь событий остаются выбором host; сообщения Styx не расширены.
