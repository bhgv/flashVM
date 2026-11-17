# Automatic terminal movie grant / Автоматический movie grant (runtime0.172)

A native launch with a FlashVM channel grant now also prepares a separate
read-only [movie acquisition service](../native/movie/README.md) **before the
process is published or executes its first instruction**. No application-specific
bootstrap callback, guessed mount name or guest monitoring loop is required.
Ordinary launches without that grant receive no movie capability.

При native launch с каналами FlashVM runtime заранее создаёт отдельную службу
загрузки над подготовленной VFS задания. `Terminal.current()` включает её в
сессию, передаваемую обычному `Terminalcmd.init(session, args)`. Управляющие
`receive`/`send` сохраняют своё назначение; запросы загрузки не смешиваются с ними.
Ни остановки чужого потока, ни cooperative polling не добавлены.

## API

| API/field | Contract / Контракт |
| --- | --- |
| [`Session.movie`](../module/terminal.m) | Independently retained `Vfsexport.Root` for this job's code/data service, present in `Terminal.current()` sessions; nil in explicit `Terminal.connect()` sessions. / Отдельный grant службы задания; explicit connect его не создаёт. |
| [`Vfsexport.terminalmovie()`](../module/vfsexport.m) | Return an exact managed root for the current process's native service, or nil without a launch; no namespace lookup or source I/O. / Выдать root своего process либо nil, без поиска по namespace. |
| [`Terminal.movies(session)`](../limbo/terminal.b) | Open one independent ordinary duplex Sys.FD on the granted service root. Nil session/root or failed native open returns nil; VM heap errors retain ordinary exception behavior. / Открыть independent duplex FD; nil/отказ дают nil, VM heap exceptions сохраняются. |

The existing [`Vfsexport.open`](vfs/cap_open.c) remains the only FD construction
path: ordinary descriptor reservation, exact result ownership, per-FD operation
ordering and deferred close apply. Each open permits one request, with the same
negotiated `flashvm.movie` S-record schema already used by the real Styx client.
The endpoint's write permission accepts **requests**; resources themselves open
read-only with actual CODE/DATA intent. A purpose/name is never a permission grant.
There is no direct native file or HTTP fallback.

A Session and its `movie` root must remain live through borrowed use. Every
`Terminal.movies()` call returns its own FD owner; the caller retains it through
I/O and retires users before dropping its last reference. Neither Root/FD objects
nor native pointers are wire representations. A remote terminal host must expose
this exact grant with the existing prepared Styx publication/transport machinery.

`Session.movie` не является сериализуемым указателем или публичным адресом. Это
обычная capability внутри DisVM. Для удалённого FlashVM хост публикует именно этот
файл существующим Styx-механизмом; SDK-клиент подключается к выданному endpoint.
Автоматическое сетевое объявление endpoint и запуск внешнего FlashVM-процесса ещё
не реализованы. Этот шаг автоматически готовит и передаёт grant внутри реального
Dis launch; сетевой путь самого SDK уже проверен отдельно.

## Namespace and lifetime / Namespace и время жизни

[`dvm_launch_process_adopt`](vfs/launch/process.c) stages the service after the
isolated namespace/event inbox has been prepared and before committing ownership.
A factory allocation failure leaves both the handoff and process unmodified.
Errors after adoption follow the existing constructor retirement path; they do
not restore consumed ownership or execute partially initialized guests.

A spawned child retains the control association and creates its **own** service
for its inherited namespace. [`Sys.pctl`](vfs/sys.c) stages a replacement service
before committing FORKNS/NEWNS. Failure preserves the old namespace, service and
system-event scope. `Terminal.current()` after success obtains the replacement.

The service retains its selected namespace group. Bind/unmount and provider
changes **within that group** remain visible to new requests. Already opened
source handles retain ordinary open-object semantics. A previously issued movie
root remains an explicit capability to its original group after the process
switches to a different namespace; it is not silently redirected. Configure the
environment first, then obtain the Session passed to init.

При spawn у child собственный endpoint. После FORKNS/NEWNS новый `current()`
использует новую группу. Ранее выданный grant удерживает старую группу как явную
capability. Изменения bind/unmount той же группы видны существующему grant без
перестроения дерева и без Limbo-мониторинга. Все native refs и source handles
освобождаются обычными путями retirement и закрытия FD.

## Execution and build / Исполнение и сборка

The runtime compiles only native VFS service and existing S-record codec sources,
using its explicit FlashVM source SDK. It imports neither a second Inferno lib9
nor FlashVM graphics/player/SDL. The [native Styx client](../native/movie/client.h)
continues to use original lib9/libstyx in its separate SDK build.

The service now accepts native `rw` and noncreating/nontruncating `rwo`, so
ordinary `Sys.ORDWR` works alongside the Styx adapter. Open/info honor pre-cancel
scopes. Data I/O deliberately does not advertise universal cancellability for
arbitrary underlying providers; existing native FD wait rules remain in force.
Close readiness follows the retained source handle. These callbacks never enter
Dis/AS managed heaps or run guest callbacks.

`Terminal.Session` gained a field and Vfsexport gained a function: rebuild matched
interfaces/libraries/consumers. Legacy builtins and mismatched module types are
not emulated by reinterpreting old frames.

## Acceptance / Проверки

[The actual startup fixture](test_launch_import.py) runs interpreter/JIT for
paired, duplex and absent terminal variants. Independent Python integer/CRC
[wire oracles](terminal_movie_frames.py) supply complete requests and exact
response prefixes; the [Limbo helper](fixtures/terminal-init/movies.b) performs
real Sys writes/reads against the automatically created endpoint.

Verified paths include:

- CODE reads provider `P`, DATA reads terminal `T` at the same virtual name;
- a child receives a service automatically, before its own application work;
- injected service-factory OOM rejects unpublished adoption without consuming handoff;
- injected FORKNS endpoint failure preserves the old usable namespace/grant;
- successful FORKNS isolates later unmounts, while an older explicit grant remains usable;
- NEWNS rebases to an explicit directory and serves a complete44-byte SWF with successful END;
- live removal denies new requests rather than exposing native host fallback;
- launches without terminal and explicit connect sessions do not invent movie authority;
- process, managed-root and FD retirement drain native provider references exactly once.

The code/data ordering fixture uses a live device-like provider yielding one byte
per read, closing after a verified prefix. NEWNS additionally serves an entire
independent SWF oracle from a real native file and verifies successful END.
Network truncation/corruption and actual FlashVM execution belong to the separate
[22-scenario TCP SDK acceptance](../native/movie/README.md#build-and-acceptance--сборка-и-проверки).
The new startup paths pass in dynamic/static native builds and under ASan/UBSan
with LeakSanitizer disabled. No physical terminal/device or complete distributed
launcher acceptance is implied.

```sh
cmake --build build/native-dynamic --target disvm --parallel
ctest --test-dir build/native-dynamic \
  -R '^disvm_native_(launch_import|terminal_init|launch_environment|code_order)$' \
  --output-on-failure
```
