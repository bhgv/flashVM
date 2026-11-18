# Native binding dispatcher / Native-диспетчер привязок

`styx_terminal_route` adds an explicitly selected native dispatcher on top of the
existing bidir control transport. It owns that transport and its sole record
reader. No Limbo loop, guest pause, guest callback, VM pointer or graphics code
is involved. Every receive operation returns at most one copied item.

## Selecting the profile / Выбор профиля

[`styxbidirbindingroute`](route.h) takes the same client and granted channel
descriptor as `styxbidircontrol`, record limit `1..65536`, mailbox slots `1..32`,
and the host's agreed UI schema identity/version. It returns an owned route or
NULL. Identity is printable non-space ASCII, at most 63 bytes; version is a
nonzero uint32. It assigns no production type name or numeric UI tag.

Use this constructor **only after negotiating the existing RX tag-zero binding
declaration profile**. Here every tag-zero record must be an existing
`fvm_sbindings` declaration. Other control protocols keep the general control
transport and their own dispatcher; this constructor does not guess their schema.
This is an opt-in host profile, not a change to 9P/Styx or the S-record wire format.

The worker validates declarations atomically and discovers matching UI tags by
identity **and** version. The original registry admits new tags and identical
repeated declarations; conflicting rebinding remains a protocol error. An
unbound record stays general even if a matching declaration arrives later.
Unknown identities and other schema versions also stay general.

RU: Конструктор явно выбирает профиль, где все входящие tag0 — уже согласованные
объявления штатного `fvm_sbindings`. Другие протоколы управления не распознаются
по догадке: для них остаётся общий control transport. Диспетчер сам применяет
объявления и направляет UI по совпадению типа и версии. Новые теги и точные
повторы допустимы; замена типа занятого тега запрещена исходным кодеком.
Неизвестные записи не пересматриваются задним числом.

## Generic typed queues / Общие типизированные очереди

[`styxbidirbindingroutes`](types.h) adds up to eight opaque typed mailboxes on
that same native dispatcher, alongside UI and general traffic. Each registration
is an agreed identity/version pair, copied at construction. Duplicate pairs,
including a duplicate of the UI pair, are refused before I/O. Array order gives
stable local indices; indices are not wire tags. Types are not assigned here.

The UI pair is optional: pass `NULL, 0` for a console/data-only session. The old
`styxbidirbindingroute` constructor keeps its UI/general behavior unchanged.
For example, using identities already agreed with the peer:

```c
StyxRouteType types[] = {{file_type, file_version}, {event_type, event_version}};
StyxBidirRoute *route = styxbidirbindingroutes(client, channels, 4096, 4,
    ui_type, ui_version, types, 2);
/* Check route before use; service general declarations and every registered queue. */
```

`styxbidirroutetypednext(route, index, &tag, bytes, capacity, &size)` copies
exactly one complete record. Its statuses and SMALL behavior match the general
reader. An empty payload is a record, not EOF. Multiple numeric RX tags declared
with the same identity/version feed the same queue in receive order. There is no
payload decoding or joining of file chunks at this boundary; permissions and
operation/correlation semantics remain with the service provider.

`styxbidirroutetypedwait(route, index, milliseconds)` optionally waits on the
native consumer, with the same monotonic budget as the UI/general wait. Each queue
has one consumer; consumers of different queues may run on different native
threads. No consumer may free the route until all other external users retire.

Every typed queue holds at most `slots` records of up to `limit` body bytes.
Additional raw storage is bounded by `count * slots * limit`, with count at most
eight, slots at most 32 and limit at most 65536. A full queue backpressures the
single dispatcher and may delay later records of other types. This is lossless
bounded storage, not a promise of latency isolation; use separately negotiated
S-record channels for bulk transfers that must not delay the control stream.
Declarations always remain general, and RX binding discovery gives no TX grant.

RU: Новый конструктор добавляет до восьми очередей identity/version. Схемы
копируются до запуска; индексы локальные, не сетевые теги. UI можно отключить
через NULL/0. Потребитель получает одну запись, включая пустую; SMALL сохраняет
её в очереди. Неизвестные типы и версии остаются общей очередью. Заполненная
очередь задерживает native dispatcher и последующие записи, но не останавливает
guest. Для независимого потока больших данных нужен отдельный S-канал.

## Mailboxes / Почтовые очереди

| API | Result |
| --- | --- |
| `styxbidirrouteui` | One copied `DVM_UIMessage`: RECORD, EMPTY or END. Output unchanged otherwise. |
| `styxbidirroutenext` | One general record: RECORD, EMPTY, END or SMALL. SMALL changes only required size and preserves the head/tag. |
| `styxbidirroutewait` | Optional native readiness wait for UI or general, within one monotonic timeout. Does not consume. |
| `styxbidirroutetypednext` | One opaque record from a registered schema's queue, including empty payloads. |
| `styxbidirroutetypedwait` | Native readiness of one registered queue; no consumption or guest execution. |
| `styxbidirroutetypedmatches` | Check a queue's immutable local identity/version registration. |
| `styxbidirroutetypedwaitcancel` | Native readiness with optional `DVM_Cancel`; cancellation consumes nothing and leaves the shared route live. |
| `styxbidirroutesend` | Copy one independently bound general/control TX record; 1 accepted, 0 refused. |
| `styxbidirroutesendwait` | Bounded cancelable native TX-capacity wait before one-record acceptance; never replays an accepted frame. |
| `styxbidirrouteuisend` | Validate/copy one `fvd1` TX frame on an independently established nonzero outgoing tag. |
| `styxbidirrouteflush` | Existing bounded TX acknowledgement wait; not a remote execution acknowledgement. |
| `styxbidirroutestate` | Dispatcher’s terminal reason, or the transport's current terminal reason while dispatch is backpressured; no queue consumption. |
| `styxbidirroutelimit` | Immutable configured record limit, or 0 for NULL. |
| `styxbidirrouteclient` | Borrow exact immutable underlying client association while the route is pinned. No permission/ownership transfer. |
| `styxbidirroutestop` | Cancel the owned workers without joining; already accepted mailbox items remain readable. |
| `styxbidirroutefree` | After external callers retire, stop/join owned workers and free storage; shared client remains owned by the host. |

There is one consumer per mailbox; UI and general consumers may be independent
native threads. Valid declarations are also copied to the general queue, so
their original payload is observable. A full mailbox backpressures the native
dispatcher at that record; the host must service both queues. Neither unknown
records nor older UI messages are silently dropped to make progress.

Malformed/conflicting declarations and malformed frames on an admitted UI tag
latch PROTOCOL, cancel native I/O and wake waiters. Previously accepted mailbox
items drain before END. Explicit stop can abandon a record not yet accepted by
a mailbox and unsent TX, as with the underlying control transport. Transport
termination is observed in stream order as the dispatcher drains earlier input.
Queues retain FIFO order individually; splitting routes does not promise a
single delivery order between independent consumers.

RX declarations never establish TX permission or tag assignment. The host passes
an independently established outgoing tag to send. Queue acceptance is not peer
execution, and uncertain writes are never replayed automatically.

RU: На очередь приходится один потребитель; UI и общую очередь могут обслуживать
разные native-потоки. Выдача всегда по одному элементу. Полная очередь задерживает
только native-диспетчер; host должен обслуживать обе очереди. Декларации остаются
видимыми как обычные записи общей очереди. Ошибка завершает приём, но уже принятые
элементы можно дочитать. Явный stop может отбросить ещё не принятый в очередь
элемент и неотправленный TX. Входящие объявления не дают исходящих прав.

## Integration and acceptance / Интеграция и проверки

The actual example is [`tests/route.c`](../tests/route.c), using the same native
launch handoff and original libstyx/lib9 peers as the underlying record tests.
It covers one-slot backpressure, short reads/writes, unknown records, delayed
binding discovery, version mismatch, conflicting declarations, malformed UI,
and stop/join with full general or UI mailboxes. It verifies the shared client
remains alive afterwards. These are native PTY tests, not new hardware PHY tests.

[`tests/typed_route.c`](../tests/typed_route.c) and its independent
[`typed_server.c`](../tests/typed_server.c) exercise mixed opaque/UI traffic,
data-only sessions, constructor copying and rejected configurations, eight schema
slots, shared-schema tags, exact NUL/binary bytes, SMALL/empty records, late
declarations, unsupported versions, conflicting bindings, full-queue stop and
parallel native consumers. Both original 9P sessions run over one fragmented PTY;
TX receipts follow independent peer byte validation. The optional C++98 fixture
links the public C APIs for engine adapters, retaining the existing 9P headers'
64-bit `long long` offset extension in that build profile. This is transport acceptance, not
execution of a remote Dis/SWF application or a file/RPC provider implementation.

This provides automatic native dispatch and copied UI mailboxes. Runtime
`$Unibus` attachment and its non-message RPCs (rows/path/ink/bulk) still need an
explicit remote endpoint integration; this library does not turn them into
remote calls by changing a queue name.

The [native host bridge](../bridge/README.md) now connects the UI mailbox to the
actual existing local `DVM_UIBridge` channels with two native relays. It retains
the general-record consumer role at the host and leaves the RPC/runtime boundary
above explicit.

The [one-shot resource client](../resource/README.md) sends the existing resource
request and receives checked reply bodies on an exclusively assigned typed queue.
It leaves UI/general consumption with the host and uses cancelable native waits.

The [autonomous resource server](../resource/SERVER.md) now consumes one request
on its own native worker, retains the prepared live namespace and streams the
actual VFS service's replies with bounded TX backpressure.

The [announced resource adapter](../resource/ANNOUNCED.md) also consumes the
existing offer schema automatically and opens independent ordinary resource fids
on that exact client. It does not install a competing raw control reader.
