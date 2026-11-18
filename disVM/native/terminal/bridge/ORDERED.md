# Negotiated terminal channels / Согласованные терминальные каналы

The existing native terminal bridge now accepts the [negotiated ordered
broker](../order/broker/README.md). Local users keep the same
[`DVM_UIBridge`](../../../include/disvm/ui_bridge.h) and copied
[`DVM_UIMessage`](../../../include/disvm/ui_message.h) APIs. This is a generic
terminal transport, usable by an IDE, ordinary applications or mixed Dis/SWF
programs. It adds no IDE layout, guest servicing loop or graphics operations.

Существующий native-мост подключается к согласованному брокеру. Локальные API
каналов и формат сообщений `fvd1` сохраняются; передавать native-указатели или
обслуживать канал из Limbo не требуется. Это общий механизм терминала, не IDE-host.

## Construction / Создание

Two native entry points are declared in [ordered.h](ordered.h):

- `styxbidirordereduibridge(peer, local_side, ui_tag)` consumes an already-owned
  broker **on every outcome**. The UI tag must already be granted and its local
  payload ceiling must hold every valid frame. Negotiation may still be pending.
- `styxbidirorderedterminal(client, channels, options)` borrows the existing
  captured launch client/descriptor, builds the ordinary control, broker and
  local bridge, then queues initial TX offer1. It returns an owned bridge or NULL.
  The caller's original client/provider/fids are never consumed.

`StyxOrderedTerminalOptions` describes local role, inner body ceiling, reorder
window, explicitly chosen outgoing outer lane, UI inner tag and the full list
of permitted inner tags. The constructor validates **all** these values before
walking/opening fids or starting workers, including duplicate tags and the UI
grant. It reserves64 outer-framing bytes. No ambient path fallback or downgrade
to a one-way channel is performed. RX lane negotiation remains independent.

The UI inner tag may be zero; it stays inside its negotiated outer envelope.
Unknown outer root0 services stay in the native service inbox, never in that UI
queue. Other predeclared tags can carry resource, file, channel or other payloads;
the bridge does not parse or consume them. Their native owner uses the borrowed
peer from `styxbidiruibridgeordered`. Never consume the UI tag separately from
its relay. The legacy route accessor returns NULL for this backend.

Both local endpoint roles work: `DVM_UI_DIS` and `DVM_UI_UI`. The relay owns the
opposite role of the same queue pair. AS wrappers remain on their FlashVM owner,
Dis adapters on their own guest owner; relay threads only copy native values.

Конструктор по launch-descriptor использует прежний captured VFS-grant, без
повторного поиска через окружение. Некорректные параметры отвергаются до I/O.
Оба варианта локальной роли поддерживаются; клиентский tag0 остаётся внутренним.
Другие grants и root-службы обслуживаются отдельно native-владельцем.

## Native construction sketch / Набросок native-подключения

This is a construction sketch for an embedding that already owns `client` and
`channels`, not a standalone CLI or a new guest interface:

```c
unsigned long tags[] = {7, 9}; /* Native grants: terminal frames and an independent resource inbox. */
StyxOrderedTerminalOptions options = {0}; /* Native-only configuration; never serialize this struct. */
options.struct_size = sizeof(options); /* Exact API descriptor version by native size. */
options.local_side = DVM_UI_DIS; /* This local endpoint is used by the Dis adapter. */
options.limit = 4096; /* Inner body, including the encoded inner tag. */
options.window = 1024; /* Per-tag future sequence displacement, not a queue length quota. */
options.lane = 71; /* Explicit native TX choice; the remote end independently proposes RX. */
options.ui_tag = 7; /* Existing fvd1 value frames, kept inside this grant. */
options.tags = tags; /* Borrowed only through construction; broker copies grants. */
options.count = 2; /* Include the terminal tag and every explicitly allowed companion. */
StyxBidirUIBridge *adapter = styxbidirorderedterminal(client, &channels, &options); /* Owned adapter/NULL. */
/* Check adapter; borrow local channels with styxbidiruibridgelocal(adapter). */
/* Retire external users, then styxbidiruibridgefree(adapter); never destroy the borrowed local pair yourself. */
```

An absent display does **not** require constructing this adapter. Existing
FD groups, stdin/stdout/stderr and ordinary native VFS operations are independent
of its optional display channels. The ordered broker itself still builds/links
without the actual UI-host libraries. The current default editor/desktop launcher
is not silently rewritten into a remote launcher: the native embedding explicitly
selects this constructor using its captured launch descriptor. This change does
not add a remote CLI transport selector, new Limbo init signature, or complete SWF
scene/physical-platform acceptance.

При отсутствии дисплея мост создавать не нужно. FD, stdin/stdout/stderr и обычный
VFS от него не зависят. Подключение выбирает native embedding; прежний desktop/editor
не превращается автоматически в удалённый launcher и не требует новых init-параметров.

## Per-client blocking TX / Блокирующая отправка своего клиента

The broker adds these [native APIs](../order/broker/broker.h):

- `styxorderedclientceiling(client)` copies the immutable local payload ceiling
  after accounting for that client's encoded inner tag.
- `styxorderedclientsendwait(client, bytes, size, timeout)` waits only for the TX
  profile and **that tag's** renewal predicate, then allocates/copies/numbers the
  packet once under the same lock. It retains one monotonic deadline across
  unrelated wakes. Timeout consumes no ordinal and never cancels a proposal.

| Result | Meaning |
|---|---|
| `STYX_ORDERED_SENT` | One immutable packet admitted; not delivery or execution ACK |
| `STYX_ORDERED_WAIT` | Selected profile/counter remained pending until deadline |
| `STYX_ORDERED_END` | Retired broker or invalid input |
| `STYX_ORDERED_SIZE` | Payload exceeds local/negotiated body capacity |
| `STYX_ORDERED_NOMEM` | Packet or first-counter allocation refused before numbering |
| `STYX_ORDERED_REFUSED` | Explicit offer rejection or exhausted counter after NACK |

The existing nonblocking `styxorderedclientsend` keeps its1/0 ABI. The terminal
TX relay uses the blocking API on its own native thread, retaining its copied
frame while waiting. Other clients and service exchange keep running. A smaller
negotiated TX limit or other permanent refusal is a terminal-adapter failure,
not a busy retry loop. The pending frame is available for diagnosis after join.

The adapter **owns its entire selected backend**. Stopping/freeing the adapter
ends that owned broker and its registered clients, while leaving the shared Styx
client and its unrelated fids alive. Independent client waits never stop it;
explicit owner shutdown or an adapter protocol failure does. Live pending
snapshots are not instructions to replay data that may already have been accepted.

Ожидающий TX-relay не приостанавливает guest/UI owner и не обслуживает чужие очереди.
Необратимый отказ размера/профиля не вызывает пустого цикла повторов. Мост владеет
своим backend целиком: завершение владельца закрывает этот backend, но не общий
Styx client и его посторонние fids.

## Retirement / Завершение

`styxorderedwatch` publishes sticky native retirement from the broker. State is
adopted only on successful subscription; callbacks may replay before return.
The source is pinned by `DVM_CancelTap`; unsubscribe consumes the tap, and final
state release follows all admitted callbacks. Callback state must fence owner
access; it may outlive the owner. No guest calls, I/O, waits or recursive owner
stop/free from a callback.

A separate lifetime fence subscribes the broker to its lower control's retirement.
Physical loss therefore wakes per-client RX/TX waits and closes the adapter's
local channels **without waiting for the broker dispatcher to run**, including
when a local RX queue is full or the TX relay awaits profile agreement. Both
fences detach owner access before destruction. Already-ready copied records
remain drainable under their existing queue contracts.

Stop is nonjoining. Join/free belong to the native lifecycle owner after external
users retire. The constructor unwinds partial control, broker, subscription,
channel and worker setup; a failed second relay does not leave the first blocked.

Нижняя подписка будит брокер и локальные каналы независимо от продвижения dispatcher.
Fence отделяет доступ до удаления owner, поэтому запоздавший callback не трогает
удалённые peer/bridge. Частично созданные workers и подписки освобождаются при отказах.

## Verification / Проверки

Original libstyx server/client, real explicit bidirectional PHY pumps, deferred
idle Tread, three-byte reads and two-byte Twrite acknowledgements. Real FlashVM
local channels cover both roles, inner tag0 isolation, a full local RX queue with
independent tag9 progress, smaller remote limit, malformed fvd1, sticky shutdown,
notification refusal, worker-creation rollback and delayed callbacks after free.
A held-dispatcher test requires physical-loss notification without RX progress.
Ordinary Tstat/fid access remains usable after optional/failed/stopped display setup.

The actual FlashVM AS channel wrappers receive and send a frame; a native gate
holds the independent TX copy until its original context/runtime are destroyed.
This is real wrapper/heap ownership acceptance, **not execution of a SWF scene**.
The C++98 header fixture links against real host libraries. Native send-wait tests
cover profile/renewal waits, exact deadlines across unrelated wakeups, independent
client progress, allocation rollback, stop wakeup and source pins after owner free.
No multi-carrier scheduler, automatic lane growth or full guest-application
migration is claimed by these tests.
