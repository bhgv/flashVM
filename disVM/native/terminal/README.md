# Bidir terminal S-record transport / Терминальные S-записи через bidir

`styx_terminal` carries the existing FlashVM S-record format on an already granted
bidir Styx terminal connection. It has independent native RX and TX workers,
bounded queues, and exactly one verified record per consumer call. It does not
interpret events, rendering, resource requests, timers or proxy-variable operations.
Those messages share this general transport through their existing/agreed type
bindings. No new S-tag assignments, event payloads or 9P dialect are introduced.

Это общий транспорт связи FlashVM ↔ DisVM в обе стороны. Ввод/вывод — часть
трафика, наряду с управлением, событиями и proxy-вызовами. Движок предоставляет
приём/отправку и очереди; конкретные схемы и обработчики остаются у диспетчера.
Никаких guest callbacks, приостановки гостя или глобального stop здесь нет.

## Native API / Native API

[`control.h`](control.h) defines an owned `StyxBidirControl`. The host first creates
an explicit bidir session, obtains a [terminal launch handoff](../launch/REQUESTS.md)
and keeps an independent client reference. Pass the SAME job/client and its
`handoff->flashvm` descriptor to `styxbidircontrol(client, descriptor, limit, slots)`.
The host must maintain this association; this adapter does not authenticate a
provider or compare arbitrary opaque factory identities.

The constructor refuses ordinary/nonwatchable clients, invalid descriptors and
limits. Body quota is1..65536bytes; there are1..32 fixed slots per direction.
Storage is bounded by `(2*slots+1)*(limit+32)` bytes plus fixed metadata and RX
scratch storage. All allocation, capture and endpoint opening finishes before
successful publication. Construction borrows inputs and retains the client on
success. Fids retain the existing attached root; no alternate host/VFS lookup.
The caller keeps the physical/bidir owner alive until all users retire.

| Entry | Meaning |
| --- | --- |
| `styxbidircontrolsend` | Copy/encode one record into bounded TX,1 accepted/0 full, invalid or stopped. Acceptance is not remote execution. |
| `styxbidircontrolnext` | Copy exactly one record: RECORD, EMPTY, END or SMALL. SMALL reports required size and preserves the head. |
| `styxbidircontrolwait` | Optional native consumer wait within one monotonic timeout; does not consume records or dispatch guest work. |
| `styxbidircontrolflush` | Wait for all currently queued native writes to be acknowledged; timeout preserves pending work. Not an application execution acknowledgement. |
| `styxbidircontrolstate` | First sticky terminal reason; already queued RX remains drainable. |
| `styxbidircontrolstop` | Request cancellation without joins or shared-client shutdown. Pending output is abandoned. |
| `styxbidircontrolfree` | After external callers retire, stop/join only its workers, clunk its fids and release storage/client pin. |

RX uses the existing untimed `styxclientreadwatch` with fresh cancellation tokens.
An idle control read therefore does not turn into ordinary request timeout. Other
RPCs can use the session's remaining capacity. Each read may contain a prefix,
one record or several; the existing `fvm_sstream` decoder retains fragments and
verifies bounds/canonical framing/CRC before publishing a record. It stops at a
full RX queue using a native condition; no record is dropped or batched into one
application event. Queued records drain after a terminal outcome.

TX uses the existing codec once per accepted record and one native writer for
the entire channel. Confirmed short writes advance only that frame's prefix;
other records never interleave. Failure, zero progress or uncertain effects stops
output; no unknown tail is replayed. Stop interrupts active read/write RPCs through
the existing Tflush cancellation path and leaves the host's client owned/alive
unless the underlying connection itself failed.

Native local-retirement subscriptions are available through `styxbidircontrolwatch`
and `styxbidirroutewatch`. They return source-pinning `DVM_CancelTap` handles,
removed with `dvm_cancel_unsubscribe`. Sticky native callbacks run outside queue
locks, can replay before subscribe returns and must remain nonwaiting, without
VM/provider/transport I/O. They report retirement without consuming queue entries;
their independently retained state may retire after transport disposal.

A clean RX EOF terminates the control object; EOF with an incomplete record is a
protocol failure. CRC/limit failure never publishes a partial record. The first
terminal reason wins; this is not a history of every concurrent failure.

## Dispatch and IDE / Диспетчер и IDE

Tag0 is delivered as an ordinary control record. The host dispatches existing
binding/control services and keeps RX/TX bindings separate. This layer neither
swallows unknown types nor treats a tag/correlation ID as access authority.
Event-specific payload validation and proxy-call completion semantics remain at
the corresponding typed dispatcher. Additional bulk/media channels continue using
the existing independent-channel negotiation architecture.

The IDE [terminal adapter](../../../ide/TERMINAL-BIDIR.md) preserves its existing
AS/Dis `fvd1` envelope and presentation/document interfaces. A host dispatcher must
route UI frames to that adapter without giving it unrelated terminal records.
No graphics primitive or editor policy is moved into this native library.

### Native IDE route / Нативный маршрут IDE

`styx_terminal_ui` exposes [ui/frame.h](ui/frame.h). It shares the standalone
[`fvd1` codec](../../host/ui_frame.c) with the existing local channel bridge.
The codec only handles counted copied values; it links no FlashVM VM, channel,
renderer or input implementation.

`styxbidiruisend(control, ui_tag, &message)` validates and copies one frame into
the existing bounded TX queue. Zero means no acceptance: invalid values, an
insufficient record limit, a full queue or a stopped transport. The caller retains
the message and can retry only an unaccepted enqueue. Queue acceptance is not an
application acknowledgement; uncertain transport output is still never replayed.

The sole general dispatcher calls `styxbidircontrolnext` once per record, handles
bindings/control, and passes an established UI route to
`styxbidiruidecode(ui_tag, tag, bytes, size, &message)`. `STYX_UI_MESSAGE` publishes
one validated copy. `STYX_UI_OTHER` leaves the borrowed record with that dispatcher
for another service. `STYX_UI_INVALID` preserves output. Neither decode result
consumes a transport record or registers another reader.

There is no reserved numeric UI tag. The host passes its established nonzero
binding; tag zero is always refused as a UI route. Binding identity does not
replace access checks. The fixture type `test.ide.fvd1` and its numeric tag are
test-only, not a newly assigned production protocol identity.

RU: Общий кодек `fvd1` используется локальными каналами и `styx_terminal_ui`.
Отправка неблокирующая, с копированием в существующую ограниченную TX-очередь.
Получение остаётся у единственного общего диспетчера: он выдаёт одну S-запись,
обрабатывает привязку и передаёт UI-запись в `styxbidiruidecode`. Чужие записи
не поглощаются, выходные значения при отказе не меняются. UI-tag согласуется
снаружи; фиксированного номера и новой схемы сообщений здесь нет.

## Verified scope / Проверено

The opt-in [native binding dispatcher](route/README.md) now owns a dedicated
record-consumer thread, applies the existing negotiated RX binding declarations,
and delivers separate bounded copied UI/general mailboxes. It does not require
a Limbo polling loop and never adds a second reader to an existing dispatcher.
The [host UI relay](bridge/README.md) connects those UI messages to the existing
real local IDE channel pair, with native-only waits, joined shutdown and retained
pending copies on failure. It supports either local endpoint role.

Actual original libstyx/lib9 peers run two opposite sessions over one PTY and a
real native launch handoff. Record input is clipped to3bytes, record writes
acknowledge2bytes; the physical adapter also fragments transfers. Tests consume
one record per call (including zero payload), preserve a too-small head, use the
existing tag-zero binding parser, exercise both directions concurrently and stop
with Tflush while the ordinary session/event path remains healthy.

Separate worker fault-injection tests cover coalesced records with a one-slot RX
queue, truncated EOF, bad CRC, quota overflow, fixed TX bounds, zero-budget flush,
short-write FIFO integrity and exactly one uncertain write attempt. These are
native unit fixtures, not additional physical-device tests.

The generic transport and native handoff path are implemented. Automatic runtime
attachment to `$Unibus`, a complete remote IDE launch, concrete timer/render/proxy
schemas, hardware PHY drivers and new bulk-channel policies are not claimed here.

The terminal peer also receives an independent `fvd1` golden frame and sends an
IDE reply checked against fixed bytes over the fragmented original 9P connection.
This covers signed int32 bounds, embedded NUL, UTF-8 and foreign/malformed route
isolation. The standalone codec fixture covers truncation and maximum payloads.
The IDE adapter checks under Node are algorithm checks; they do not claim remote
IDE or FlashVM script execution. Current validation results belong to the tested
revision, rather than an invariant test count embedded in this document.

The [automatic outgoing offer producer](resource/OUTGOING.md) observes native
resource-source changes and emits the existing declaration plus READY/CLOSED
S-records. The [announced receiver](resource/ANNOUNCED.md) opens independent
ordinary resource fids of the same client. Both preserve shared route ownership
and use native cancellation without guest participation.

Transport retirement now [wakes route mailboxes directly](route/RETIREMENT.md),
without waiting for the sole dispatcher to regain queue capacity. The UI relay
also observes retirement and closes its own local channels to wake idle/full
native waits. Accepted records remain individually drainable; no guest pause,
new event format or shared-client shutdown is introduced.
