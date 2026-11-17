# Remote physical carriers / Физические носители удалённой связи

Remote communication is not tied to TCP, UDP, sockets or POSIX descriptors.
[`phy.h`](phy.h) defines an owned native carrier boundary; [`select.c`](select.c)
selects from a host-supplied driver table. The existing asynchronous
[Styx client](../disVM/native/client/README.md) now uses this boundary for read,
write, wait, local wake, heartbeat traffic and disconnect. `styxclientmountfd`
remains a compatibility adapter; `styxclientmountphy` accepts a custom carrier.

Связь отделена от физического носителя в корневой папке `remote_phy/`, общей для
будущих потребителей DisVM/FlashVM. В generic API нет требований иметь IP-адрес,
сокет или fd. SPI, I²C, FireWire, USB, RS-485, CAN, Bluetooth, UART/RS-232 и закрытые
радиопротоколы подключаются своими native-драйверами и, при необходимости,
адаптерами потокового транспорта. Конкретные аппаратные драйверы здесь пока не
добавлены: доступен Linux POSIX byte-descriptor driver и проверяемый custom API.

## Layers and ownership / Слои и владение

| Layer | Responsibility / Ответственность | Current scope / Текущий объём |
| --- | --- | --- |
| VFS/namespace | Visible names, live provider stacks and actual rwx access. / Видимость и права. | Existing DisVM capability/union rules unchanged. |
| 9P/Styx | Files, fids, request tags, T/R transactions and Tflush. / Файловые операции. | Original lib9/libstyx codecs; native client and alternative original server use PHY. |
| Control S-records | Typed announcements, events and application messages. / Управляющие записи. | Existing outbox/monitor; no new wire tags here. |
| Stream/logical adapters | Framing, ordered reliable delivery, reassembly, logical-channel arbitration as needed. / Адаптация носителя. | Composable drivers and [two-stream native mux](mux/README.md); general packet/reliable tunnel remains future work. |
| RemotePhy | Native read/write/wait/wake/availability/connect/disconnect and capabilities. | Implemented without OS handles in public ABI. |
| Driver family | Device/bus/IRQ/descriptor details. / Конкретная физика. | [`drivers/posix/fd.c`](drivers/posix/fd.c); other families can be added separately. |

Each physical carrier has one native I/O owner. Only `wake` may be called
concurrently while the carrier is pinned. A driver implements nonblocking
read/write and an interruptible wait; it can use an IRQ, RTOS event, condition,
USB completion queue or host descriptor internally. Native work/cancellation must
wake the wait even if signalled just before it starts. No guest cooperation,
VM/SDL callbacks or global stop is involved.

Driver callbacks must not reenter the owning protocol engine. Wait runs outside
that engine's lock; read/write/wake must remain bounded and native-only. The
native protocol owner keeps its existing absolute deadlines and quotas. PHY
callbacks do not grant namespace access or authenticate a peer. The host chooses
and grants the carrier; the existing admission/VFS layer still decides what the
peer may see and do.

## API

| Entry | Contract / Контракт |
| --- | --- |
| `phycreate(ops, state, info)` | Owned carrier/NULL; state adopted only on success. Copied ops/capabilities. |
| `physelect(table, count, name, params, size)` | Exact host-selected factory; ambiguous/missing names refuse before effects, no global registry or fallback. |
| `phyconnect(phy, params, size)` | Driver-specific native setup; parameters borrowed through return. No assumption that an address is a network URL. |
| `phyread`, `phywrite` | One nonblocking precise operation; count is a confirmed prefix. Respect max_read/max_write hints, never replay an uncertain tail. |
| `phywait(phy, wanted, timeout, &ready)` | Wait/poll for READ/WRITE and local WAKE/HANGUP/FAILED; timeout0 polls,-1 waits until an event. No fd parameter. |
| `phywake(phy)` | Thread-safe nonwaiting local interrupt, not an on-wire ping. |
| `phyavailable(phy, &bytes)` | Optional available-byte hint, UNSUPPORTED when unknown; zero is neither EOF nor permission. |
| `phyinfo` | Local capabilities/MTU/limits/lifecycle snapshot, owner-serialized. |
| `phydisconnect`, `phyfree` | Close wire once; keep wake state until all users retire, then consume driver storage. |

A positive read returns data or explicit AGAIN/EOF/error; a successful zero-length
operation does not invoke the driver. Partial AGAIN retains its confirmed prefix.
Malformed/overflowing/missing-prefix acknowledgements become uncertain failure.
Styx treats uncertain output as terminal instead of replaying bytes. A confirmed
complete reply received before a carrier error remains an acknowledged result;
other outstanding requests fail according to existing flush/side-effect rules.

Read/write callbacks operate on bytes. Raw packet drivers advertise PACKET and
must be adapted appropriately before use by the stream client. The driver owns
native parameter validation, connection details and partial-setup cleanup.
An already connected carrier accepts only an empty idempotent connect; supplying
new parameters refuses instead of silently retargeting a live connection. Final
destroy can run on a retirement caller after the I/O worker exits. The public
header is C89/C++98 compatible; native implementation uses C99.

## Stream adaptation / Адаптация потоков

Native Styx admission requires connected **STREAM + ORDERED + RELIABLE**
capabilities. A bare UDP/CAN/radio packet carrier is not silently treated as a
stream. Depending on the carrier, an adapter must provide framing, fragmentation/
reassembly, ordering, duplicate suppression and loss handling, or report failure.
The PHY capability bit is a driver contract; declaring it does not implement
those mechanisms. 9P request tags and its length field do not replace them.

Полудуплексность допустима: арбитраж и очередность приёма/передачи находятся ниже
логического потока. Для SPI/I²C ведомый обычно получает возможность отправить
данные при обращении ведущего; driver/adapter должен использовать доступные IRQ,
attention line, опрос или временные слоты. Задержка физической шины сохраняется.
Ожидающий Tread избавляет от ожидания прикладного heartbeat, но не создаёт
физическую возможность передачи в обход правил шины.

A wrapper may own another RemotePhy and forward/transform its byte operations,
readiness and wake. This permits adding a framing/reliability layer without
changing the 9P client. The [PTY acceptance wrapper](../disVM/native/tests/phy/limited.c)
limits reads to3 and writes to2 bytes, forwards native interrupts, and injects a
precise terminal-prefix error. It is a test adapter, not a radio reliability stack.

## Default and optional sessions / Обычный и дополнительный режим

The default is one ordinary Styx session: use `styxclientmountphy` and
`styxphyinit` directly on the carrier. Replies and pending event Treads already
provide return traffic. No P9MX queues, pump or framing are created on this path.

Two opposite sessions require the **separate explicit `phymuxcreate` call at both
peers**; see [mode selection](mux/README.md#explicit-opt-in--явное-включение).
The client's outstanding-request limit and heartbeat do not activate this mode.
There is no automatic upgrade of an ordinary stream. Most applications can keep
the ordinary path; mutual VFS requests are the reason to opt into the extension.

## Bidirectional requests / Двусторонние обращения

Standard 9P is a bidirectional byte connection with **client T-requests and
server R-replies**. A server may delay a Tread while processing other requests.
A pending control Tread therefore lets it deliver an S-record immediately when
an event/request payload becomes available, without waiting for a heartbeat.
The existing [outbox](../disVM/native/movie/OUTBOX.md) and
[monitor](../disVM/native/movie/MONITOR.md) already use this path for announcements.
The receiver reissues its watch read after consuming data. During the short gap,
bounded server-side buffering retains pending data.

Наш heartbeat сейчас использует Tflush на незанятый oldtag и стандартный Rflush.
В Rflush нет поля данных: добавлять туда запрос или «флаг необходимости обратиться»
без нового согласованного протокола нельзя. Для содержательных сообщений служит
обычный control stream. Произвольный общий обратный RPC пока не оформлен; наличие
фонового reader само по себе не означает готовый механизм удалённого вызова.

For **mutual standard VFS access**, the [implemented optional mux](mux/README.md) provides two independent
logical 9P sessions: A-client→B-server and B-client→A-server. Each direction has
its own fids/tags, authentication/admission and prepared exported root. Both may
share one physical carrier through an explicit logical-channel demultiplexer
with one physical I/O owner. This needs no second physical port or IP listener.
Both peers explicitly select the P9MX v1 transport extension; ordinary 9P remains
unchanged within each stream. This does not automatically publish/mount roots. Mixing reverse T-messages into the current ordinary
client session would be rejected as invalid replies.

For **application messages**, one granted read/write control service can carry
requests/replies/events in both directions using S-records and correlation IDs.
This is a possible higher-level extension, separate from mutual 9P mounts and
from carrier drivers. It must not derive permissions from a correlation ID or
heartbeat observation. No per-SWF special client program is required by this
architecture; a common host dispatcher would perform the routing.

References: [9P intro](https://9p.io/magic/man2html/5/intro),
[read/write](https://9p.io/magic/man2html/5/read),
[Tflush/Rflush](https://9p.io/magic/man2html/5/flush).

## Build and verified scope / Сборка и проверки

Standalone carrier library, without Inferno/FlashVM/player dependencies:

```sh
cmake -S remote_phy -B build/remote-phy
cmake --build build/remote-phy --parallel
ctest --test-dir build/remote-phy --output-on-failure
```

The native SDK links the same tracked library via its CMake target. Actual PHY
acceptance includes a no-descriptor custom driver, selector/configuration/
lifetime/acknowledgement checks, and native original-codec 9P transactions over a
raw PTY, with no TCP/UDP/socketpair. PTY responses are fragmented; a stacked
adapter caps I/O further. Additional cases preserve a final known reply across
carrier error and reject unreliable carriers before emitting 9P bytes.

The existing SDK's socket/multiplex/heartbeat/cancel/event/VFS and56 actual-player
TCP cases remain enabled through the fd compatibility adapter. Full Linux64
native SDK acceptance passes normally and under ASan/UBSan, LeakSanitizer disabled.
Physical hardware, RF loss recovery, hardware drivers, dynamic channel allocation,
hardware-backed mux and migration between physical links are not claimed tested here.

The original imported POSIX server remains available as `infernostyx`.
The [alternative original-core server backend](../disVM/native/server/phy/README.md)
now accepts a granted carrier through `styxphyinit`, with complete-frame staging,
bounded reply queues, producer wake pins and original deferred-read retirement.
Select `infernostyx_phy` for that backend; no vendored code or wire message is
changed. Hardware drivers and automatic reverse-session setup in remote Dis launches remain
subsequent work. Runtime/FlashVM versions and guest module interfaces are unchanged.
