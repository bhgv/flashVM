# P9MX v1 transport extension / Расширение транспорта P9MX v1

This is a project-specific, explicitly selected framing layer over a connected
reliable ordered byte carrier. It is **not a new 9P message**, not a modification
of Rflush and not auto-detected on a stock Inferno connection. Both native hosts
must configure this adapter. Byte payloads preserve ordinary 9P unchanged.
The native Linux implementation is [pump.c](pump.c); public ownership is in
[mux.h](mux.h). Channel roles and application integration are in [README.md](README.md).

## Frame

All multibyte values are **unsigned little-endian**. No C struct layout is sent.
Frames may be fragmented at any byte by PHY. They are not interleaved on the wire.
There is no padding, checksum, pointer, credential or native descriptor field.

| Offset | Bytes | Meaning |
| --- | --- | --- |
| 0 | 4 | ASCII `P9MX` |
| 4 | 1 | Version `1` |
| 5 | 1 | Type: DATA=`0`, CLOSE=`1`, CREDIT=`2` |
| 6 | 1 | Channel `0` or `1` |
| 7 | 1 | Reserved, must be zero |
| 8 | 4 | Payload byte length |
| 12 | length | Payload |

| Type | Payload | Effect |
| --- | --- | --- |
| DATA | 1..1024 uninterpreted bytes | Append to that channel's receive stream; charge receive allowance. |
| CLOSE | Exactly zero bytes | End that logical channel in both directions after its preceding data. |
| CREDIT | Exactly four bytes: positive byte allowance | Add transmit allowance for this channel, independently of the other channel. |

Maximum complete frame size is1036bytes. A header is validated before its payload
is read. Unknown magic/version/type/channel, nonzero reserved bits, invalid length,
zero/excessive credit, data beyond allowance and any frame after that sender's
CLOSE fail the entire mux. No scanning/resynchronization or fallback is attempted.
The reliable carrier supplies integrity/order/loss handling; P9MX adds none.

## Credits and bounds / Кредиты и границы

Each side initially sends one CREDIT per channel, equal to its own bounded RX
capacity1024..1048576. DATA may be selected only after receiving allowance. The
sender subtracts payload bytes when selecting a frame, not when it eventually
finishes writing it. Confirmed partial physical writes retain their exact prefix.
No selected frame is replayed following an uncertain or terminal transfer.

The receiver tracks its own granted remaining allowance. DATA must fit both that
allowance and free RX capacity. Consuming received bytes accumulates a CREDIT to
send back. At all times, for an open channel, queued RX + outstanding allowance +
pending grant equals local capacity. Peer CREDIT additions must be positive and
leave remaining transmit allowance at most1048576. Peers may use different local
capacities; there is no implicit symmetric-size assumption.

Credits are flow control, **not permission** or operation acknowledgements. A
malicious peer can terminate its connection by violating them, but cannot make
the local queues exceed their configured capacity. Application/9P validation and
access control still apply inside each logical stream.

A blocked sender retains its bounded TX queue while the other channel continues.
The native pump alternates eligible frames and sends pending credits without
waiting for application data. One complete bounded output frame may already be
selected when a close/stop/remote failure is observed.

## Close and failure / Завершение и ошибки

Local disconnect aborts unselected queued TX, then sends CLOSE after the already
selected frame, if any. No subsequent frames are generated for this locally
closed channel. Peer DATA already in flight remains bounded by outstanding
allowance and is discarded if the local application has already disconnected.
Remote CLOSE drops unselected TX; confirmed RX remains readable, followed by EOF.
The other logical channel remains live.

`phymuxfree` aborts the whole link, joins its native worker and disconnects the
physical carrier. It does not guarantee pending CLOSE/data delivery. For graceful
shutdown, acknowledge application work and coordinate both peers before abort.
A physical/protocol failure is shared by both channels. Complete confirmed input
already published remains readable; incomplete frames are discarded. Outstanding
9P operations decide their outcome using their ordinary replies/cancellation/
uncertainty rules. There is no automatic physical reconnection, migration or replay.

Никаких новых прав, корней VFS или способов входа этот формат не создаёт. Свои
пространства имён и подготовленные корни стороны назначают обычным 9P-сессиям.
Возможные будущие динамические каналы, согласование версий, защита транспорта и
переподключение потребуют отдельного явно согласованного расширения.
