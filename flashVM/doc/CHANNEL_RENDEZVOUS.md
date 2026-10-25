# Blocking channels and observable wait directions

[Русский](CHANNEL_RENDEZVOUS.ru.md) · [Public API](../include/flashvm/channel.h) ·
[Implementation](../src/capi/channel_c.c) · [Shared Dis host](../../disVM/host/README.md)

The existing in-process channel remains a copied-value FIFO by default.
[`fvm_channel_receive`](../include/flashvm/channel.h#L110) blocks on empty; [`fvm_channel_send`](../include/flashvm/channel.h#L106) blocks when a configured
capacity is full. `try_send`/`try_receive` do not wait for a peer. Calling blocking
operations from a VM/render owner can stall that owner; use an appropriate worker.

`fvm_channel_set_rendezvous(channel, 1)` selects explicit handshake behavior on an
empty, open and idle local channel. Blocking send publishes one offered message,
then waits until a receiver removes that exact node. Capacity settings do not
add a buffer in this mode. Multiple producers/consumers are supported, with no
fairness guarantee. Nonblocking send needs a currently waiting receiver and an
empty, unfrozen transfer slot; otherwise it returns [`FVM_NOT_FOUND`](../include/flashvm/flashvm.h#L49). Successful
try-send commits an offer to that ready peer; it does not wait for the peer's
native thread to resume. Receivers can use borrowed, owned or graph/mixed APIs.
A mismatched primitive receiver still cannot consume a queued graph packet.

Close wakes both sides. A blocked receiver with no deliverable offer returns
[`FVM_ERROR`](../include/flashvm/flashvm.h#L45). A blocked send whose offer was never received withdraws it and
returns [`FVM_ERROR`](../include/flashvm/flashvm.h#L45); caller ownership of a failed graph send is preserved. If
receipt already committed, send returns success even if close immediately
follows. Existing buffered queues still drain queued messages after close.
Every channel owner must retain a handle until its operation/thread completes;
last release/destroy must not race users. Debugger freeze blocks receipt; thaw
or close wakes waiters.

`fvm_channel_wait_state(channel, &state)` observes a single mutex-protected
snapshot without waiting for data, capacity or a peer. It briefly locks the
channel; it is not a lock-free API. The output contains pending message count,
waiting sender and receiver counts, closed/frozen/mode flags and direction bits
[`FVM_CHANNEL_WAIT_SEND`](../include/flashvm/channel.h#L21) / [`FVM_CHANNEL_WAIT_RECEIVE`](../include/flashvm/channel.h#L22). Both direction bits can be
set. Counts include native condition waits and the balanced cancellable
ActionScript polling scopes. They are observations, not reservations, thread
identities or permission to assume the next try operation will succeed.

[`fvm_channel_available`](../include/flashvm/channel.h#L139) / `pending` continue to inspect queued data. A frozen
queue may have pending data that is not yet deliverable. `waitingSenders()`,
`waitingReceivers()` and `waitDirections()` expose the new observations on an
ActionScript channel wrapper; they return numbers without blocking for a peer.
The existing `ready()`, `pending()`, `trySend()` and `tryReceive()` stay available.
Script waits retain existing cooperative cancellation and debugger bookkeeping.
Native rendezvous is configured explicitly by the embedding host; this series
does not add a script mode-changing operation or remote wire opcode.

Wait snapshots and rendezvous configuration are local-queue operations.
File/remote channels return [`FVM_UNSUPPORTED`](../include/flashvm/flashvm.h#L48), preserving snapshot output;
a local proxy must not claim knowledge of remote native waiter threads.
Changing mode while messages/waiters exist or after close returns
[`FVM_INVALID_ARGUMENT`](../include/flashvm/flashvm.h#L47). No struct layout or native pointers enter a wire format.
This extension is not a Styx/9P compatibility claim.

Validation includes sender-first/receiver-first handshakes, nonblocking failure,
receipt/close cancellation, buffered capacity waits and 800 transfers between
four native threads. Original channel/owned/capacity/shared/close/value/graph and
ActionScript extension regressions remain part of the checks. Linux is the
executed platform; Windows/macOS/Android use the existing corresponding port
implementations and still need target execution acceptance.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_channel_available` | function | [include/flashvm/channel.h:139](../include/flashvm/channel.h#L139) |
| `fvm_channel_receive` | function | [include/flashvm/channel.h:110](../include/flashvm/channel.h#L110) |
| `fvm_channel_send` | function | [include/flashvm/channel.h:106](../include/flashvm/channel.h#L106) |
| `fvm_channel_set_rendezvous` | function | [include/flashvm/channel.h:48](../include/flashvm/channel.h#L48) |
| `FVM_CHANNEL_WAIT_RECEIVE` | macro / define | [include/flashvm/channel.h:22](../include/flashvm/channel.h#L22) |
| `FVM_CHANNEL_WAIT_SEND` | macro / define | [include/flashvm/channel.h:21](../include/flashvm/channel.h#L21) |
| `fvm_channel_wait_state` | function | [include/flashvm/channel.h:41](../include/flashvm/channel.h#L41) |
| `FVM_ERROR` | enum constant | [include/flashvm/flashvm.h:45](../include/flashvm/flashvm.h#L45) |
| `FVM_INVALID_ARGUMENT` | enum constant | [include/flashvm/flashvm.h:47](../include/flashvm/flashvm.h#L47) |
| `FVM_NOT_FOUND` | enum constant | [include/flashvm/flashvm.h:49](../include/flashvm/flashvm.h#L49) |
| `FVM_UNSUPPORTED` | enum constant | [include/flashvm/flashvm.h:48](../include/flashvm/flashvm.h#L48) |
<!-- source-links:end -->
