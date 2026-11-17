# Two logical streams on one carrier / Два потока на одном носителе

`remote_phy_mux` is an **optional extension below ordinary 9P/Styx**. It turns one
explicitly granted reliable ordered byte carrier into two independent logical
`RemotePhy` streams. It does not alter 9P messages, add paths, publish a root or
grant access. Both peers must explicitly select this framing before use. A stock
Inferno peer uses its ordinary stream directly, without this adapter.

Это расширение транспорта, а не изменение Styx. Одна сторона может одновременно
иметь клиент для VFS другой стороны и сервер своей опубликованной VFS. Обе стороны
могут послать T-запрос сразу: ожидать ping/pong или оповещение «теперь спроси меня»
не требуется. Настоящая задержка полудуплексной шины и её арбитраж остаются у PHY.

The exact frame layout, credit invariants and failure rules are documented in
[WIRE.md](WIRE.md).

The public API uses `bidir` to make this opt-in visible at the call site:
`PhyBidir`, `phybidircreate`, `phybidirchannel`, `phybidirfree`, declared in
[`bidir.h`](bidir.h). The `phy` prefix reflects that this adapter carries byte
streams and does not itself initialize Styx sessions. Ordinary mount/init names
and behavior remain unchanged. The earlier `phymux*` names are replaced, without
ambiguous aliases; the P9MX v1 wire format is unchanged.

## Explicit opt-in / Явное включение

Ordinary single-session Styx is the default for applications. Pass the connected
carrier directly to `styxclientmountphy` on the client and `styxphyinit` on its
server. Neither constructor creates this mux, sends P9MX frames, starts its pump
or allocates its queues. Their existing RPC concurrency and heartbeat options do
not enable reverse sessions.

| Mode | Explicit construction | Request initiators |
| --- | --- | --- |
| Ordinary, default | Direct carrier to the ordinary Styx constructor | Client sends T-requests; server returns R-replies. |
| Two opposite sessions, opt-in | `phybidircreate` at both peers, then ordinary Styx constructors on its channels | Each peer has its own client and server. |

Для большинства применений достаточно обычного режима. Отдельный вызов
`phybidircreate()` — точка явного включения расширения; добавлять флаг к обычному
mount/init для этого не требуется. По умолчанию встречная сессия не создаётся.
Настройка `limit` у клиента задаёт число одновременных запросов одной сессии,
а не включает встречную сессию. Ожидающий Tread для событий тоже работает в
обычном режиме и не требует P9MX.

«Однонаправленный» здесь относится к инициированию запросов. Обычная 9P-сессия
уже передаёт байты в обе стороны: T-запросы и R-ответы. Две встречные сессии нужны,
когда обе стороны должны самостоятельно обращаться к опубликованной VFS другой.

There is no automatic upgrade or protocol sniffing. Configure both peers before
starting the carrier's protocol owner. Opt-in framing does not itself publish
anything or authorize reverse mounts. Any future remote-launch integration must
retain ordinary mode as the default and require an explicit selection for the
opposite-session setup; that launch option is not implemented yet.

## Roles and API / Роли и API

| Channel | Endpoint A | Endpoint B |
| --- | --- | --- |
| `0` | Ordinary native 9P client | Original libstyx PHY server |
| `1` | Original libstyx PHY server | Ordinary native 9P client |

Channel numbers are explicitly matched by the host, not inferred from 9P packet
contents. Each 9P session retains its own version, fid/tag space, authentication,
prepared exported root, rwx checks and cancellation. Namespace union/order remains
the VFS layer's responsibility. A channel ID and receive credit confer no access.
The two streams can also carry a different host-selected byte protocol; the mux
never decodes their contents or assumes 9P frame boundaries.

```c
#include "mux/bidir.h"
/* carrier is consumed on every outcome; each queue gets 16 KiB. */
PhyBidir *link = phybidircreate(carrier, 16384);
/* Check link and returned endpoints for NULL before using them. */
RemotePhy *outgoing = phybidirchannel(link, 0);
RemotePhy *incoming = phybidirchannel(link, 1);
/* Pass outgoing to styxclientmountphy and incoming to styxphyinit.
 * Each constructor consumes its endpoint. On peer B, exchange 0 and 1.
 * Retire protocol owners, then phybidirfree(link). */
```

[`bidir.h`](bidir.h) documents ownership and threading. `phybidircreate` accepts connected
STREAM+ORDERED+RELIABLE carriers and queue capacity1024..1048576, consumes the carrier
on success or failure, allocates all four bounded byte queues and starts one native
physical I/O owner. Capacity may differ between peers. Bare packets are refused.

`phybidirchannel` takes channel0 or1 exactly once under the mux owner; it returns an
owned logical carrier or NULL. Each logical carrier then has its own native I/O
owner, including an independent interruptible wait and sticky local wake. Wake uses an
atomic generation plus a Linux futex wake without taking a producer mutex;
generation comparison prevents losing an interrupt sent just before wait. Only
wake is concurrent on that logical carrier. Other channel operations and the
physical pump run independently through short native mutex sections. There are
no guest callbacks, guest polling, guest suspension or global stop.

`phybidirfree` is an explicit **abort**, not a delivery acknowledgement: it requests
stop, wakes and joins the physical worker, disconnects the wire and drops the mux
owner. Unsent queues may be discarded. For orderly application shutdown, peers
must first acknowledge their own work and coordinate protocol/channel retirement.
Surviving endpoint/wake pins keep native storage alive after pump retirement.
Their reads drain already confirmed input then report failure; final endpoint
release frees carrier storage. Do not call free on the physical worker itself.

## Bounds and flow control / Ограничения и управление потоком

Each channel has separate RX and TX rings of the configured local capacity. The
physical worker additionally owns one incoming and one outgoing frame of at most
1036bytes each. Memory is bounded; stream writes accept a confirmed **local queue
prefix**, never claim delivery or remote execution, and return AGAIN when full.
A selected physical frame frees local TX space before it is delivered; it remains
bounded worker-owned storage through partial writes. Higher-level 9P replies,
not local enqueue counts, acknowledge file operations.

Per-channel receive **credits** prevent a slow or unread channel from occupying
the other channel's receive queue or blocking its control frames. Each endpoint
initially grants its RX capacity. Reading bytes replenishes that channel's grant.
The physical pump only selects DATA within the peer's granted allowance, returns
credits independently of data, and alternates eligible channel frames. Channel0
may be full while channel1 continues in either direction. This is bounded fairness,
not a latency guarantee on a busy physical bus. A failed underlying carrier ends
both channels; it is a shared physical fault domain.

На каждый канал — отдельные лимиты. Пока потребитель не читает, новые кредиты не
выдаются и передача данных именно этого канала останавливается на его bounded TX.
Второй канал, CREDIT и CLOSE продолжают обслуживаться. Native wait не держит mutex
во время физического ожидания; поступление local work пробуждает physical owner.

Local disconnect aborts only its channel: queued unselected TX is discarded; an
already selected frame finishes before CLOSE. Peer input confirmed before CLOSE
remains readable, followed by EOF. Peer CLOSE disables further logical writes.
CLOSE is terminal for that channel ID: reopening/reusing it requires a fresh mux.
No per-channel reconnect, link migration or automatic retry is performed.

## P9MX v1 wire extension / Формат расширения

This is a project transport extension, **not standard 9P, S-record framing or an
Inferno negotiation message**. The host must configure it at both ends. There is
no sniffing, fallback to plaintext 9P, magic scanning, checksum, cipher or session
resumption. Reliability, integrity and peer admission remain below/above this
adapter as appropriate. Do not silently claim reliability for a lossy raw bus.

Every frame has this12-byte header; integer fields are unsigned little-endian.

| Offset | Bytes | Meaning |
| --- | --- | --- |
| `0` | `4` | Literal ASCII `P9MX` |
| `4` | `1` | Version `1` |
| `5` | `1` | Type: DATA=`0`, CLOSE=`1`, CREDIT=`2` |
| `6` | `1` | Channel `0` or `1` |
| `7` | `1` | Reserved, must be zero |
| `8` | `4` | Payload byte count |

DATA carries1..1024 uninterpreted bytes, within granted receive credit. CLOSE has
no payload. CREDIT has exactly4 payload bytes, a positive additional byte allowance.
The sum of remaining peer credit cannot exceed1048576. Credits are independent
per direction and channel; local receive capacity remains the final bound.
Unknown version/type/channel, reserved bits, invalid lengths, zero/overflowing
credits, data beyond granted capacity or any frame after peer CLOSE terminally
fail the whole mux. There is no recovery that might reinterpret a damaged frame
as another request. Physical read/write fragmentation is retained exactly.
Uncertain physical output is terminal and is never replayed.

## Verified scope / Проверено

The native SDK acceptance runs two actual processes over **one raw PTY**, each
using the original lib9 codec, original libstyx server and native client. Physical
reads are capped at3bytes and writes at2bytes. Both sides have pending event reads
while ordinary identity-file RPCs continue; native producers complete one event
per read. A second scenario cancels one watch with actual Tflush while the reverse
session completes its event and the cancelled session continues ordinary RPCs.
Both peers witness logical CLOSE before the fixture allows physical stop.

Independent API tests use a local socketpair, asymmetric1024/2048 capacities,
exhausted receive credit, a full TX queue, live opposite-channel traffic, binary
65537-byte ring wraparound, sticky early wake, independent channel close and late
pins after pump join. A raw peer supplies malformed headers/credits/sequences
without using the production encoder. A descriptor-free driver confirms that
uncertain physical output is attempted exactly once and that failed admissions
consume their carriers without wire effects. Fixtures do not pause guests.

The native pump currently builds on Linux with pthreads and monotonic futex
waits; the base RemotePhy ABI remains carrier independent. All 296 Linux64 native
SDK tests pass normally and under ASan/UBSan, with LeakSanitizer disabled. The
public mux header also compiles as C89/C++98.

This is native transport plumbing plus actual original-9P acceptance. It does
not yet automatically wire reverse session roots into a remote Dis launch,
change the existing FlashVM terminal protocol, allocate dynamic bulk channels,
provide encryption or implement a SPI/I²C/CAN/radio driver. Those can build on the
explicit carrier/stream boundary without altering standard 9P sessions.
