# Confirmed tag changes / Подтверждённая смена тегов

Native scope operations now support ACK-gated TX activation and an ordered
receiver-side RX commit. Only tags listed in the operation are held. Other
clients continue sending and receiving; an unrelated service ACK waiting for TX
capacity does not hold their traffic.

The mechanism uses the existing outer S-record tag0. Wire payload encoding is
an explicit native Unibus adapter responsibility. No numeric command IDs, global
application tags or production ACK schema are assigned by these C APIs. The
fixture's text commands demonstrate ordering; applications do not need their own
special-purpose negotiation program.

RU: Запрос назначения ставится после уже принятых старых записей. Новый трафик
изменяемого тега ждёт подтверждения. Другие теги и клиентские очереди продолжают
работать. Глобального ожидания всех клиентов или каналов нет.

## Ordered root services / Упорядоченные root-службы

[`styxscopeservicehandler`](../service/service.h) installs a trusted native
handler for **transport root tag0**. The sole broker invokes it after preceding
outer records have been admitted and before processing the next outer record.
Client-local and group-local zeros never invoke this hook. Unknown root services
return PASS and remain in the normal native root inbox. HANDLED consumes the
service record; ERROR retires the scoped broker.

The handler runs outside hub locks and may call native assignment/operation
functions or queue a reply. It must not wait, join, execute guests, perform
provider/wire I/O or free its owner. Replacement adopts state only on success.
An independently pinned context survives an admitted callback, including
self-replacement. Final release runs outside locks after its last invocation.

`styxscopeservicesend` copies a root0 body into a dynamic native outbox. Its
separate native sender may wait for bounded control TX capacity while the broker
keeps servicing RX. Already admitted control frames retain their FIFO order.
Unrelated application TX may interleave; a service record is not a global
application stop. The local source retiring wakes this sender and all operation
owners through existing native conditions/cancellation.

## Sender / Отправитель

[`operation.h`](operation.h) provides these native host operations:

| Function | Effect |
| --- | --- |
| `styxscopetxprepare` | Copy candidate assignments/request, reserve affected tags and enqueue root0 request |
| `styxscopetxconfirm` | Apply matching ACK or NACK; never release a hold for a foreign/stale result |
| `styxscopetxstate` | Copy current operation ID and native diagnostic state |
| `styxscopetxwait` | Block only the native owner for an exact operation ID; timeout does not alter state |

The caller supplies a nonzero increasing correlation ID per native parent.
Candidate native capabilities, expected local generations and request bytes are
copied before admission. Proposal allocation/validation failure changes neither
mapping, reservations nor retired operation IDs. Only admitted IDs are retired.
The request reaches control TX after already accepted old frames.

Only selected tags are held. Changing a group tag affects its descendants because
they use that envelope; its unrelated sibling groups remain independent. A held
child protects the ancestor wrapper it actually uses. Pending new tags are
reserved against conflicting native assignments, while unrelated sibling
assignments remain valid. One proposal per parent is supported; disjoint parents
can have simultaneous operations, including identical local correlation IDs.

Matching ACK atomically commits the copied TX candidate and releases those holds.
Matching NACK preserves the previous TX map and releases them. Exact repeats of
the most recently completed outcome are idempotent, even during a newer proposal.
Conflicting repeated outcomes, unrelated IDs and older uncached confirmations
return refusal without changing the current operation. Waits target an exact ID;
another operation's ACK cannot satisfy them or make them wait on a newer request.

A timeout is not NACK. No automatic retry or rollback resumes traffic after an
unknown peer outcome. The host must reconcile the operation or retire the root;
late confirmation cannot revive a retired broker. Local correlation/generation
values are not implicitly serialized native struct layouts or access rights.

## Receiver / Получатель

`styxscoperxcommit` preallocates the copied ACK before changing any RX mapping.
Under one mapping lock it validates/commits the sibling transaction and adopts
that ACK into the native outbox. Returning 1 means both local steps committed,
not that the peer already received the reply. Failure returns 0 with RX unchanged,
allowing an explicit native NACK to be queued safely.

An ordered root handler calls this before returning HANDLED. Therefore following
records are admitted under the new mapping. The peer emits affected new-tag data
only after receiving its ACK. Old accepted client messages already belong to their
inboxes and remain readable under their original recipient, without reinterpretation.
The request adapter must validate authorization, identify the native parent and
handle duplicate requests/cached outcomes before calling commit. This generic
mechanism does not infer those wire semantics or provide a receiver replay cache.

## Sequence numbers and multiple carriers / Номера и несколько носителей

For one ordered S-record stream the queued request provides the activation
boundary. There is no need to wait for unrelated clients or drain every channel.
If a tag is striped over several carriers, restore order **for that tag and its
scope/direction** before applying its queued assignment request.

The selected design is a **64-bit record sequence** plus a separate **64-bit
assignment generation**, scoped to tag namespace and direction. A 32-bit sequence
is not the default or an implicit truncation. Modular serial comparison requires
the outstanding window to remain below 2^63. A separate generation/session
boundary distinguishes old incarnations after reassignment or reconnect.
At ten million records per second the sequence wraps after approximately
58,000 years. The existing canonical S-integer codec can represent this magnitude
without serializing native machine layout; numeric tag IDs remain separate.
These are selected design widths, not new wire fields implemented by this change.
No per-tag reorder buffer, striping engine or automatic carrier growth is claimed here.

## Verification / Проверка

Five [original-Styx operation scenarios](tests/host.c) run real native bidir
carriers, original server/client RPCs, three-byte reads and two-byte write ACKs.
An old TX record reaches the peer before its proposal. The peer sends ACK only
on an explicit release; new TX tags are accepted only afterwards. Tests cover
NACK preserving old tags, missing ACK, stale/conflicting/duplicate confirmations,
operation-specific waits, copied candidates and local-zero isolation.

While A waits for its ACK, B sends on its unchanged tag. A nested C operation
with the same correlation ID and candidate tag confirms independently while D
continues sending. Ancestor wrapper and candidate-tag reservations are checked
without freezing unrelated namespaces. A receiver ACK sender is deliberately
held before control admission; broker RX and unrelated B TX still progress.
The actual peer emits new RX-tag data only after receiving that ACK.

Other cases reject an invalid RX map, fail ACK allocation before RX mutation,
fail proposal/handler allocation, replace a native handler during its own admitted
callback, detach it explicitly, withhold confirmation and retire through local
stop, real client shutdown or malformed service. Native ownership witnesses check
exact state release. Existing scope tests additionally cover sender-worker and
broker-worker constructor rollback and C++98 public-header linkage.

Negative controls remove the affected-tag hold, restore a global parent hold,
restore a global service-outbox hold, accept a wrong ACK ID or mutate RX before
ACK-copy allocation succeeds. Each mutation fails its corresponding native
original-Styx assertion. No altered production body is used in passing runs.

The production wire adapter must scope request IDs to the connection incarnation
and native parent, validate its message schema and retain receiver outcomes for
its retry policy. Local counters alone cannot authenticate or distinguish a
replayed message from an older connection/session. This implementation intentionally
does not infer that missing wire agreement from opaque root-service bytes.
