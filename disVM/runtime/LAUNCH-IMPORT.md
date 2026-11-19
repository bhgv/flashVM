# Native launch import / Принятие native launch (runtime0.170)

[Launchcontext](LAUNCH-CONTEXT.md) now makes copied accepted startup label/bytes
available to the current bootstrap and its children, including display-free launches.

Runtime0.172 adds [automatic Session.movie acquisition grants](TERMINAL-MOVIES.md),
including child inheritance, namespace replacement and ordinary Sys.ORDWR streams.

The runtime can now adopt a prepared launch handoff **before publishing the first
process or executing its bootstrap**. The namespace and FlashVM channel grant
move into the new process. `Terminal.current()` opens the inherited connection
directly and returns the existing typed Session for the application's init.
No channel mount, capture or namespace pathname is needed for this import.

Теперь host constructor принимает prepared handoff до первой инструкции guest.
Процесс получает готовый namespace и отдельную capability связи с FlashVM.
Bootstrap вызывает `Terminal.current()` и передаёт Session обычному typed
`Terminalcmd.init(session, args)`. Специализированный клиент приложения не нужен.

[Caller-local module lookup](CODE-LOOKUP.md) now lets the trusted host set the
native handoff's `local_role` before adoption; zero keeps provider startup behavior.
The lookup role is immutable per process and independent of execution admission.

## Trusted host entry / Вход trusted host

A native launcher can call `dvm_runtime_main_with_handoff(argc, argv, handoff)`
once per process. Use the ordinary CLI arguments with a host-selected local
bootstrap. The launcher keeps the handoff shell alive through runtime shutdown
and frees it after return. The ordinary `disvm` executable calls this entry with
a null handoff. `--gdb` cannot transfer an in-process handoff across exec.

Нативный лаунчер может один раз на процесс вызвать
`dvm_runtime_main_with_handoff(argc, argv, handoff)` с обычными параметрами CLI
и локальным bootstrap, выбранным лаунчером. Оболочка handoff остаётся у вызывающего
до возврата после завершения runtime. Обычный `disvm` вызывает этот вход без
handoff. Режим `--gdb` не переносит handoff между процессами.


[`vfs/launch/runtime.h`](vfs/launch/runtime.h) exposes the native startup boundary:

| API | Ownership and phase |
| --- | --- |
| `dvm_runtime_launch_module(module, handoff)` | ACTIVE startup controller supplies an already loaded provider-local **zero-argument bootstrap** and optional handoff; returns the published initial process, with dispatch handled by the existing controller |
| `dvm_launch_process_adopt(process, handoff)` | Constructor-only adoption into a fresh initialized **unpublished** process; returns1/0; no guest execution |
| `dvm_launch_process_inherit(child, parent)` | Private process initialization copies channel names and retains the granted provider for the child |
| `dvm_launch_process_release(process)` | Owner retirement or failed private setup releases the process's channel pin |

`module` is borrowed through the launch call. The constructor retains a reference
before `linkmod`, which consumes that reference. The caller retains its original
reference on success and exception and must eventually release it with `unload`.
This permits a controller to retry preparation failures without using a freed
Module. The ordinary CLI controller now releases its original reference on both
paths. It still passes NULL handoff for ordinary startup.

This interface is a trusted native embedding boundary, not a guest syscall or
foreign-process control API. It accepts no PID and cannot replace a running
process environment. The caller owns private process storage through construction;
adoption rejects published/native-owner state and repeated adoption. Native
structures and Module/provider pointers must never be copied onto a wire.

Нельзя применять adoption к работающему guest или использовать его как способ
приостановки/перенастройки чужого потока. Это этап создания нового процесса.
Код приложения исполняется на provider VM; модуль для исполнения на терминал
не скачивается. Проверку аутентификации и выбора provider-local модуля выполняет
trusted admission adapter.

## Commit, failure and retirement / Commit, ошибки и завершение

Adoption first validates optional channel metadata, allocates immutable process
channel storage and creates a new automatic namespace event inbox. Before commit,
a failure leaves both the old private process environment and the handoff intact.
At commit it moves `handoff.group` and its channel provider into the process and
clears those handoff fields. Remaining description/argument bytes stay owned by
the caller's handoff shell until `dvm_launch_handoff_free`.

A later publication failure disposes of the adopted environment through ordinary
private-process cleanup. It does **not** restore a consumed handoff. The caller
may free its empty shell, but must obtain a new handoff for another launch.
Successful construction uses the existing PID/message registration and permanent
native owner model. There is no global stop, ready-queue migration, target polling
or cooperative guest callback.

The startup frame transaction also cleans up its native scratch frame, unpublished
stack extent and module link when construction raises. Controller REG source
scratch is restored. The initial frame is a zero-argument bootstrap frame; no
application arguments have been published into it. On success stack ownership
moves to the registered process before temporary construction storage retires.

Дочерний поток наследует копию неизменяемых имён receive/send и собственный
provider pin. Его namespace и `/dev/sysevents` продолжают жить по обычным правилам
процесса. Изменение bind/union не подменяет terminal capability. Managed Root,
Session и открытые FD имеют независимые ссылки и освобождаются своими штатными
механизмами; native provider уничтожается после ухода последнего владельца.

## Managed interface / Managed-интерфейс

| Extension | Behavior |
| --- | --- |
| `Vfsexport.terminal()` | Exact managed Root for the current process's inherited channel grant; nil when absent |
| `Vfsexport.terminalpath(direction)` | Copied immutable receive0/send1 name; nil for absence or another direction |
| `Terminal.current()` | Obtain that root/names and use the existing capability-open helper; owned Session/nil |

The current process's immutable association makes the separate root/name reads
consistent. Lookup performs no provider I/O. Opening Session channels uses the
existing native path cancellation boundary and ordinary Sys.FD ownership. Each
`current()` call opens a new Session according to the endpoint provider's open
semantics; there is no hidden session cache. A bootstrap normally obtains it
once and passes that Session to the application.

These are DisVM extensions. Standard Inferno Sys remains unchanged. The existing
S-record framing, directional schemas, Styx transport and extra-stream operations
remain the same as [runtime0.169](TERMINAL-INIT.md).

```limbo
# Bootstrap already runs inside the adopted environment.
term->init();
session := term->current();
if (session == nil) {
    raise "terminal unavailable";
}
app->init(session, args);
```

## Executable evidence / Исполняемая проверка

The [bootstrap](fixtures/terminal-init/import.b) uses a real constructor-adopted
namespace and channel grant. A native fixture supplies only an explicit local
`/pkg` directory for compiled provider-side modules; this is a test grant, not
implicit host fallback. After loading interfaces, the guest removes `/pkg` and
all root mounts. `Terminal.current()`, child inheritance and typed application
init still work. The channel grant is never mounted or captured.

The interpreter/JIT driver runs paired, duplex and absent-terminal variants.
It checks initial environment visibility, nil/invalid direction behavior, ordinary
startup without a handoff, child `killwait`, binary/control/bulk traffic through
the existing application fixture, and final native channel/namespace retirement.
Before dispatch, each variant also injects malformed metadata and inbox allocation
failure; terminal variants inject channel-storage OOM. A separate publication
failure occurs after successful adoption. Observers at real `newstack/free`
boundaries verify that rejected attempts free both the native scratch frame and
unpublished stack. The original handoff then launches successfully.

Verification runs all 104 native/runtime cases and all 41 host-unit cases in both
dynamic and static builds. A separately ASan/UBSan-instrumented runtime runs the
launch-environment, typed-terminal and launch-import cases, including interpreter
and JIT variants. LeakSanitizer is disabled (`ASAN_OPTIONS=detect_leaks=0`) because
it fails in this environment; explicit native allocation/retirement observations
cover the tested startup frames, stack extents, channel roots and streams.

The test endpoint is a native in-memory provider, not an executing FlashVM or
network listener. An authenticated transport must still deliver a native handoff
to the trusted controller. Network capability encoding, remote module-interface
marshalling and startup argument decoding are not added here. The host entry
constructs the generic bootstrap; its ordinary typed call supplies the application's
init arguments. No native frame injection into arbitrary application signatures
is claimed.

Принятие native handoff и импорт terminal capability теперь подключены к реальному
созданию процесса. Сетевой admission adapter, wire-формат capability и общие
удалённые вызовы module interfaces остаются следующими этапами.
