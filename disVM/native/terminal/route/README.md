# Native Unibus dispatcher / Native-диспетчер Unibus

`styx_terminal_route` owns one native incoming dispatcher on an explicitly bidir
Styx terminal connection. The broker blocks for incoming S-records; independent
consumers block for their own recipient queues. Dynamic recipient queues never
make the broker wait for application consumption. No guest callback, polling,
forced suspension or alternate interpreter is involved.

See [dynamic recipients and system services](DYNAMIC.md) for admission, memory
failure, tag-zero handling, queue metrics and the scope of verification.

## Setup / Создание

[`styxbidirbindingroute`](route.h) selects an agreed UI identity/version.
[`styxbidirbindingroutes`](types.h) optionally adds up to eight opaque typed
recipients and supports console/data-only sessions with NULL/0 UI identity.
Schemas are copied during setup; duplicate pairs are refused before I/O.

```c
StyxRouteType types[] = {{file_type, file_version}, {event_type, event_version}};
StyxBidirRoute *route = styxbidirbindingroutes(client, channels, 4096, 4,
    ui_type, ui_version, types, 2);
/* Check route before use; each recipient has its own consumer. */
```

The client and descriptor must identify the same actual native Styx grant.
[Captured-root validation](../GRANTS.md) checks exact client, local ceiling and
canonical relative endpoints. Resource walks remain under that root; file-only
roots cannot acquire sibling services. `styxbidirroutewalk` returns an owned
unopened fid; callers pin the route through the call and retire before free.

`limit` remains 1..65536 and `slots` 1..32 for native transport staging. Recipient
storage grows with unread messages instead of being limited to `slots` entries.
Each queue has one consumer; distinct consumers may run on distinct native
execution threads. UI decoding and opaque copying never cross managed heaps.

## Routing / Маршрутизация

| Incoming record | Destination |
| --- | --- |
| Tag0, existing version1 bindings | Atomic broker registry update plus legacy general observation copy |
| Tag0, another canonical subservice/version | Separate native service inbox; its handler validates the body |
| Nonzero tag bound to selected UI identity/version | Copied decoded UI queue |
| Nonzero tag bound to registered typed identity/version | That recipient's opaque queue |
| Other nonzero tag/type/version | General queue |

Bindings are directional. Exact repeats are idempotent; conflicting bindings and
malformed admitted UI terminate the route. Unknown application records are not
reclassified after a later declaration. RX associations grant no TX permission.
Non-binding service bodies are preserved, not executed by guessing their format.

RU: Брокер применяет bindings, служебные записи оставляет native-службам, а
nonzero-теги направляет адресатам. У каждого consumer своё блокирующее ожидание.
Накопление одной FIFO не мешает другим очередям или следующим tag0.

## Receiving and diagnostics / Приём и диагностика

| API | Behavior |
| --- | --- |
| `styxbidirroutenext` | One general record, including legacy binding observations; SMALL preserves head |
| `styxbidirrouteui` | One copied UI message |
| `styxbidirroutetypednext` | One opaque record from registered recipient index |
| `styxbidirrouteservicenext` | One non-binding tag-zero body for a native channel-service handler |
| `styxbidirroutewait` | Block native caller for selected general/UI queue readiness |
| `styxbidirroutetypedwait` | Block native caller for its typed queue readiness |
| `styxbidirroutetypedwaitcancel` | Same with independent caller cancellation |
| `styxbidirrouteservicewait` | Block native system-service consumer for its inbox |
| `styxbidirroutequeueinfo` | Non-consuming coherent current/peak records and payload bytes |

All receive calls deliver at most one record. Empty payload is a record, not EOF.
Waits preserve one monotonic budget; cancellation consumes nothing and does not
cancel the shared route. See [`traffic.h`](traffic.h) for system inbox and metrics.

## Sending and lifetime / Передача и владение

`styxbidirroutesend`, `styxbidirroutesendwait` and `styxbidirrouteuisend` retain
existing whole-record TX admission, independent outgoing tags and precise-prefix
acknowledgement. Acceptance is not remote execution. Unknown effects are not
replayed. `styxbidirrouteflush` waits for native queued output acknowledgements.

Stop/failure wakes recipient waits; all accepted records remain individually
drainable before END. Genuine allocation/accounting exhaustion reports NOMEM.
The [retirement subscriptions](RETIREMENT.md) and
[client-terminal watch](../LINK-RETIREMENT.md) remain independent of consumer
progress. Final free requires external users to retire, stops/joins owned workers
and frees unread copied nodes. Local stop/free does not close the host's shared
client. Late native callbacks use detached private lifetime contexts.

## Integration / Интеграция

The [native UI bridge](../bridge/README.md) relays between its UI recipient and
the actual local FlashVM/DisVM channels. A blocked local relay does not stop the
shared dispatcher. The [resource client](../resource/README.md),
[resource server](../resource/SERVER.md),
[announced adapter](../resource/ANNOUNCED.md) and
[outgoing publisher](../resource/OUTGOING.md) retain their own consumers and
cancellation/lifetime contracts.

The original Styx fixtures cover dynamic accumulation, late bindings, opaque
system service records, exact FIFO bytes, actual allocation failure, independent
consumers, cancellation, malformed/conflicting input and late callback safety.
The C++98 public-header fixture checks native engine linkage. These tests do not
claim automatic remote `$Unibus` RPC integration, execution of remote applications
or general bulk/media channel negotiation. `sysmessages` format and automatic
queue-growth warnings remain to be agreed and implemented separately.
