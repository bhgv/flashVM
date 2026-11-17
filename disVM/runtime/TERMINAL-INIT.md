# Typed terminal init / Типизированный terminal init (runtime0.169)

Runtime0.170 adds [native constructor adoption and Terminal.current](LAUNCH-IMPORT.md),
superseding the native-to-managed import limitation below. The original executable
mount/capture example remains a separate capability-open regression test.

The optional [Terminalcmd interface](../module/terminalcmd.m) accepts the existing
FlashVM communication endpoint explicitly:

```limbo
init(terminal: ref Terminal->Session, args: list of string)
```

The provider executes the application in its own VM. The terminal parameter
contains ordinary managed byte-stream FDs and an explicit channel-root capability.
Local and remote FlashVM use this same interface; the granted provider determines
transport and endpoint location. No application-specific client is required.

Это расширение интерфейсов DisVM, а не изменение стандартного Inferno Sys ABI.
Существующие module entry без аргументов сохраняются. Общий bootstrap готовит
окружение и передаёт `Session` в `init` через обычный типизированный вызов Limbo.
Графика, ввод и шрифты обслуживаются FlashVM через существующие записи сообщений.

## Explicit capability opening / Открытие capability

`Vfsexport.open(root, relative, mode)` opens an already granted root directly and
returns an ordinary owned `ref Sys->FD`, or nil. It neither mounts that root nor
searches the caller's namespace. It works for any suitable VFS provider, including
channel/socket/device-backed nodes and ordinary files. The provider decides what
the name opens and enforces current access policy.

| Input | Meaning |
| --- | --- |
| `root` | Exact native-created `Vfsexport.Root`; its grant survives original mount removal |
| `relative` | Canonical relative name; `.` means the granted node; absolute/empty/traversal components refuse |
| `Sys->OREAD` | Existing readable node |
| `Sys->OWRITE` | Existing writable node, without create/truncate |
| `Sys->ORDWR` | Existing bidirectional node |

Other mode bits, including OTRUNC and ORCLOSE, refuse before provider I/O. A nil
root or failed open returns nil and replaces any prior result owner correctly.
The existing bounded descriptor registry and deferred cleanup apply; nil may
also report descriptor quota or allocation failure. Include `sys.m` before
`vfsexport.m` in direct callers; the Terminal interface supplies its dependencies.
Native factory status is reflected in the existing `%r` diagnostic. There is no
fallback to another provider or host path after a refusal.

The current owner captures the guest path before native admission. The root stays
pinned by the mapped builtin argument, and the unpublished managed FD has an
explicit GC root across native I/O. Descriptor reservation/publication, per-FD
operation serialization, cancellation admission, normal Sys I/O and deferred
close all use existing runtime mechanisms. No target polling, debugger pause or
cooperative guest callback is introduced.

## Generic terminal library / Общая библиотека терминала

[`terminal.m`](../module/terminal.m) and [`terminal.b`](../limbo/terminal.b) define:

| API | Result |
| --- | --- |
| `Terminal.init()` | Load matched Vfsexport builtin before concurrent library use |
| `Terminal.connect(root, receive, send)` | Owned Session/nil; equal names open one ORDWR FD, distinct names open a receive/send pair |
| `Terminal.open(session, name, mode)` | Independent FD for a live additional stream through the same explicit root |

`Session.root` retains the grant; `Session.receive` and `Session.send` retain
ordinary FDs. With duplex, both fields refer to the same FD owner. With paired
channels, each FD has its own native ownership. A failed second open releases
the staged input through normal managed ownership cleanup. The caller keeps the
session and each borrowed FD live throughout operations and joins its users
before releasing the last owner.

Направления определяются относительно выполняемого Dis: receive — от FlashVM,
send — к FlashVM. После сообщения с именем дополнительного потока приложение
открывает его через `Terminal.open` либо `Vfsexport.open(session.root, ...)`.
Дерево каналов остаётся живым; заранее фиксированного списка потоков нет.

The byte streams carry the existing [FlashVM S-record protocol](../../flashVM/doc/SRECORD.md).
The terminal helper does not add a codec or infer message boundaries from read
sizes. Keep directional binding tables separate and use the existing fragment
receiver and bulk-size negotiation where applicable. A Styx-backed provider
transports these bytes through ordinary Styx read/write operations; S-records
are not a replacement Styx wire format. Concrete control commands announcing
additional channels remain the endpoint protocol's responsibility.

## Bootstrap and evidence / Bootstrap и проверка

The executable [bootstrap](fixtures/terminal-init/main.b) loads the application
through the generic Terminalcmd interface, prepares the environment, captures an
explicit channel grant, removes its original namespace mount, opens the terminal
session and calls the application's typed init. The [application](fixtures/terminal-init/app.b)
transfers binary data, receives a dynamically offered stream name and opens that
stream without a namespace dependency. It requires no device-specific logic.

The native fixture feeds this through the real launch prepare/take API before
application init. Its `/link` mount is fixture-selected, not a reserved production
device or a mandatory temporary mount. Production adapters may expose an already
granted root using their ordinary VFS admission path. The channel root stored in
the native0.168 handoff is still a native capability; this release does not add
an automatic native-to-managed capability importer to the host bootstrap.

The real dedicated interpreter/JIT test covers duplex aliasing, separate paired
FDs, fragmented binary reads, independent dynamically offered bulk traffic,
readonly grants, path/mode refusal, repeated FD-result replacement, nil-root
failure, capability lifetime after unmount and complete native stream/root drain.
The test connection is an in-memory native provider, not a running FlashVM or
an authenticated network listener. The default host entry still has no args;
the typed application entry is called by the provider-side Limbo bootstrap.

Native `dvm_terminal_channel_open` also now translates its public `r/w/r+/rw`
spellings into the provider's existing `r/wo/rw` modes. This fixes compatibility
with Inferno-style provider adapters and avoids a create/truncate request.

The extended acquisition-wait test also blocks direct capability opens on native
condition variables. External `cancelwait`/`killwait` interrupt those callbacks;
an open acknowledged by the provider retains its successful result when racing
cancellation. All21 native cancellation contexts and all acquired FD/factory
owners retire, including the private negative control for a missing cancel scope.

Остаются сетевая передача capability, автоматическое принятие launch handoff
в host bootstrap и общая инфраструктура удалённых module interfaces. Новый
typed init уже проверяется как настоящий вызов Dis, но полного сетевого запуска
эта версия не заявляет.
