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

## Mailboxes / Почтовые очереди

| API | Result |
| --- | --- |
| `styxbidirrouteui` | One copied `DVM_UIMessage`: RECORD, EMPTY or END. Output unchanged otherwise. |
| `styxbidirroutenext` | One general record: RECORD, EMPTY, END or SMALL. SMALL changes only required size and preserves the head/tag. |
| `styxbidirroutewait` | Optional native readiness wait for UI or general, within one monotonic timeout. Does not consume. |
| `styxbidirroutesend` | Copy one independently bound general/control TX record; 1 accepted, 0 refused. |
| `styxbidirrouteuisend` | Validate/copy one `fvd1` TX frame on an independently established nonzero outgoing tag. |
| `styxbidirrouteflush` | Existing bounded TX acknowledgement wait; not a remote execution acknowledgement. |
| `styxbidirroutestate` | Dispatcher’s first terminal reason; no queue consumption. |
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
single delivery order between the two independent consumers.

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

This provides automatic native dispatch and copied UI mailboxes. Runtime
`$Idehost` attachment and its non-message RPCs (rows/path/ink/bulk) still need an
explicit remote endpoint integration; this library does not turn them into
remote calls by changing a queue name.
