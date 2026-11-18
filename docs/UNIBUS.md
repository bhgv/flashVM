# Unibus

Unibus is a universal host/service interface for Dis, SWF and combined applications.
The IDE is one application using it. Applications execute on the same DisVM and
FlashVM engines as other programs; there is no IDE-specific interpreter or ISA.
The canonical Limbo interface is [`unibus.m`](../disVM/module/unibus.m), loaded
as `$Unibus`. The interface was renamed throughout the source; there is no old-name alias.

Unibus — общий host-интерфейс приложений Dis, SWF и их комбинаций. IDE использует
те же движки и общие службы, что остальные приложения. Специализированного
интерпретатора IDE нет. Имя модуля — `Unibus`, builtin — `$Unibus`.

## Application portability / Переносимость приложений

An application package consists of portable `.dis`/`.swf` modules, `.m` interface
descriptions and resources. Platform-specific implementations belong to the VM,
its native adapters and system providers. Moving a package between supported
systems with compatible VM formats/interfaces must not require recompiling that
application or writing a special per-application remote service program.

Shared variables and proxy images, task/thread creation and retirement, load-based
growth/shrinkage, extra bulk/stream channels and remote execution are general
runtime mechanisms. Their policy is not selected by recognizing an IDE module
name. This change does not implement a new automatic scaling policy.

A display/FlashVM session is optional. Console modules may receive nil in their
typed application `init` and use inherited FD 0/1/2 and other permitted VFS nodes
normally. Absence of UI authority does not disable stdin, stdout or stderr.

Пакет приложения переносится как Dis/SWF-код, интерфейсы и ресурсы. Под систему
собираются движки и системные адаптеры. Разделяемые значения, прокси, потоки,
масштабирование и каналы — общие службы для всех приложений. Консольный запуск
с nil display/session остаётся штатным, с обычными унаследованными FD.

## One namespace, different providers / Одно пространство, разные провайдеры

The filesystem frontend is a view of the existing VFS, with its live namespace,
bind/mount layers and permissions. It must not create a second filesystem model
or a snapshot of an entire tree at connection time. A prepared connection gets
its permitted live root; the provider decides what that root exposes.

The native Styx terminal adapter and its announced resource loader both resolve
from that [captured grant root](../disVM/native/terminal/GRANTS.md). Matching
ambient names cannot replace granted endpoints. The adapter checks exact client
association and local access ceilings; ordinary server operations still decide
current permissions. A grant for one file does not authorize sibling services.

Project files can come from local storage, another machine, multiple machines,
Git, or several Git revisions simultaneously. Ordinary path operations retain
their meaning. Provider properties expose origin, selected revision and stack
position for inspection/reordering; application code need not infer the physical
kind of a node from its name. A device, channel, socket, ordinary file or service
endpoint remains an ordinary permitted namespace node to its consumer.

Git commit views can be read-only layers; a writable working layer can receive
edits according to the existing union policy. Reading a moving branch and reading
a fixed revision are distinct provider configurations. Mixing revisions does not
bypass module interface/version validation. Executable resources keep the agreed
provider-first lookup policy; ordinary data uses its configured namespace order.

VFS-фасад показывает то же живое пространство имён, права и слои bind/mount.
Файлы проекта можно собрать из нескольких машин, репозиториев и ревизий Git.
Расположение и ревизия — свойства провайдера; дерево остаётся изменяемым.
Read-only ревизии могут сочетаться с разрешённым рабочим слоем для записи.

## File facade and framing / Файловый фасад и S-записи

The proposed FlashVM facade includes the user's `file` and `state` nodes: writing
SWF bytes to `file` prepares the module; writing `run` to `state` starts it.
Control/statistics nodes for both FlashVM and DisVM can be exposed through the
same VFS view. Final paths, command/error schemas and completion semantics still
need specification; this document does not claim those provider nodes exist yet.

On shared Unibus channels, requests, replies, file data, SWF chunks, events,
statistics, proxy operations and control traffic are all inside **S-records**.
Bindings identify agreed payload types; no raw SWF/file bytes may be inserted
between unrelated records. A file facade encodes ordinary FD operations at its
boundary, so applications need not hand-encode S-records merely to write a file.
Large/streaming transfers may use separately negotiated channels rather than
monopolize the common control path. Additional Unibus channels also carry
S-records; creating a channel does not implicitly permit unframed bytes. Each
record remains independently identifiable, so unrelated messages can coexist
without a receiver treating their contents as part of a file or SWF stream.
No new numeric S-tags are assigned here.

Styx/9P continues to supply files/channels and its normal protocol messages;
this does not redefine Twalk/Topen/Tread/Twrite. The current binding-declaration
RX profile is explicitly selected, not silently imposed on every future tag-zero
control schema. Any broader control dispatcher must preserve other negotiated
types and messages.

Operations on FlashVM run on its owner through copied messages. A foreign thread
does not inspect/mutate its heap directly. Event/statistics delivery does not
pause guests or require a Limbo polling loop. The VFS provider still makes the
final access decision when an operation executes.

В общем канале всё передаётся внутри S-записей, включая данные файлов и SWF;
сырых вставок между ними нет. Фасад сам кодирует обычные FD-операции. Styx/9P
остаётся транспортом файлов и каналов. Дополнительные каналы Unibus также
сохраняют S-обрамление; выделение канала само по себе не разрешает сырые данные.
Запись `file`, затем команда `run` в
`state` — согласованное направление файлового фасада, а не реализованный сейчас
набор узлов с уже зафиксированным протоколом команд.

## Implemented boundary / Реализованная граница

The runtime now accepts a native Unibus binding in a launch handoff **before
the first application instruction**, resolves calls on the current guest owner,
supports explicit inheritance and retains native service lifetime independently
from the initial thread by default. The [.m call/ownership API](../disVM/runtime/host_interface/README.md)
documents implemented policies and failure behavior.

Existing native S-record transport, typed routing and the real host UI channel
relay remain reusable mechanisms. The [typed dispatcher](../disVM/native/terminal/route/README.md)
also provides independently consumed bounded opaque schema queues alongside UI
and general records, including sessions without UI. It does not interpret file
or RPC payloads or assign their schemas. Native test callbacks exercising launch metadata
are not remote RPC implementations. Generic RPC marshalling, filesystem control
providers/Git views and the remaining portable IDE presentation integration are
separate work. The legacy desktop embedding still contains application-specific
native code and is not proof of a fully portable IDE package.

The existing resource protocol now has a [shared per-request reply receiver](../disVM/native/movie/REPLIES.md)
used by the Styx loader and available after typed Unibus routing. It validates
BEGIN/BYTES/END, stages exact bytes and publishes the origin only on explicit
success. This is one-resource reply handling, not a new correlated file/RPC API
or implementation of the proposed `file/state` control nodes.

A [one-shot native resource client](../disVM/native/terminal/resource/README.md)
now sends the existing request through an independently granted TX binding and
receives replies from an exclusively assigned typed queue. It has one monotonic
budget, native cancellation and private staging of the returned origin. The
[autonomous native server](../disVM/native/terminal/resource/SERVER.md) now handles
the provider side through the actual VFS service, retaining its live prepared
namespace and respecting bounded TX backpressure. Automatic logical-channel
negotiation, correlation and `file/state` control nodes remain separate work.

The [announced resource adapter](../disVM/native/terminal/resource/ANNOUNCED.md)
now receives existing ready/closed generation offers on a native typed consumer.
Repeated and parallel resource fetches open independent ordinary service fids on
the exact same connection, preserving VFS decisions and S-record framing. This
uses the existing offer and per-open protocols; general bulk/video/audio channel
negotiation and the proposed control nodes remain separate integration work.

The [automatic outgoing offer producer](../disVM/native/terminal/resource/OUTGOING.md)
now connects native source changes to shared TX using existing declaration and
READY/CLOSED S-records. It has independent source subscriptions, bounded native
TX admission, source/route retirement wakeups and no guest polling. Local producer
stop does not withdraw a shared source; orderly withdrawal closes the source and
publishes CLOSED. Queued generation/DONE means queue admission, not peer receipt.

[Native route retirement](../disVM/native/terminal/route/RETIREMENT.md) now wakes
queue/resource waits even when the dispatcher is blocked by a full mailbox.
The real local UI relay also closes its owned channels on route retirement,
including idle/full-channel cases. Accepted messages remain individually drainable;
private callback contexts detach object access before disposal. Shared Styx clients
and caller cancellation sources remain independently owned.

[Active resource I/O](../disVM/native/terminal/resource/ACTIVE-IO.md) now receives
route retirement through the server's own native cancellation scope. It reaches
cancellation-aware provider operations, preserves other scopes of the same
provider, and keeps route FAILED distinct from explicit server STOPPED. Native
cleanup and late callbacks retain their ordinary independent ownership; no guest
polling, suspension or new wire control schema is involved.

[Client fetch cancellation](../disVM/native/terminal/resource/FETCH-CANCEL.md)
now reaches active ordinary resource RPCs and native staging sinks. Announced
fetches use independent child scopes: a cancelled caller does not retire the
receiver or another fetch. Readiness is interruptible without consuming offers,
and raw-session origin publication now follows the final generation fence.

Native transport [connection retirement](../disVM/native/terminal/LINK-RETIREMENT.md)
is automatic even with full RX/route queues and idle TX. Already admitted records
remain individually drainable. Native resource workers observe retirement without
guest polling or additional application-level heartbeat messages.
