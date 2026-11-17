# Managed movie service / Управляемая загрузка SWF

This SDK connects the actual FlashVM managed player to a prepared DisVM VFS over
**original Inferno libstyx/lib9, standard 9P2000 and S-records**. No new 9P message
or per-SWF client program is introduced. Code and variable reads use the same
existing player logic and one generic callback.

SDK соединяет настоящий managed player FlashVM с подготовленной VFS DisVM через
**исходные Inferno libstyx/lib9, стандартный 9P2000 и S-записи**. Стартовый SWF,
вложенные SWF и переменные загружаются общим механизмом, без отдельной программы
для каждого SWF. Код запрашивается с provider-first назначением, данные — в обычном
порядке union. Расширение касается содержимого отдельного выданного byte stream;
формат 9P и существующий терминальный control stream не меняются.

## Host API / API хоста

| API | Contract / Контракт |
| --- | --- |
| [`styxmovieservice(group)`](service.h) | Retain the host-authorized prepared namespace and return an owned provider, or NULL. Its fixed root can be captured/bound as one file and accepts duplex `rw` opens; every open owns one independent acquisition. / Удержать подготовленный namespace, вернуть свой provider либо NULL; независимая загрузка на каждый duplex open. |
| [`styxmoviefetch(...)`](client.h) | Install directly as `FVM_DesktopManagedFetch`, with a borrowed retained `StyxClient` as userdata. Return FVM_OK only after explicit successful END; propagate sink refusal, otherwise fail acquisition. / Установить общим callback загрузчика; успех только после END, отказ sink сохраняется. |
| [`styxbacking(provider)`](../vfs/file_provider.h) | Adapt the granted service file to the existing original-libstyx server/publication integration. The backing owns its provider reference. / Адаптировать выданный файл к существующему серверу/publication; backing удерживает provider. |

The host prepares and authorizes `group` **before** exposing the service. The
constructor retains this live group; it does not copy a fixed resource tree or
export the whole provider machine. Tattach usernames, filenames and S-record
bindings supply no additional authority. Use the existing authenticated/prepared
publication path when admitting external peers. The test server is a local,
unauthenticated acceptance fixture, not that admission implementation.

Хост заранее готовит и разрешает namespace конкретного задания. Удерживается живая
группа: каждый запрос заново разрешает путь, а уже открытый ресурс имеет собственное
владение. Можно скрыть, заменить и перебиндить ресурсы обычными средствами VFS.
Имена, usernames и binding не расширяют права. Сам endpoint допускает запись
запросов; операции над запрошенными ресурсами в этой версии только read-only.

Host integration sketch (not a complete launcher):

```c
FVM_DesktopManagedLoader loader;
fvm_desktop_managed_loader_init(&loader);
loader.fetch = styxmoviefetch;
loader.userdata = granted_client;
loader.max_loads = resource_budget;
status = fvm_desktop_run_source_managed(&options, &source, &loader, &error);
```

`granted_client` is an owned/retained [StyxClient](../client/client.h) whose attached
root is **the granted service file**. Keep it, source/options and transport state
alive through the synchronous run. Each fetch clones a fresh fid, opens ORDWR,
negotiates directional bindings, requests one resource, consumes its reply and
clunks on every outcome. Never serialize native sink/userdata pointers.
Client and source callbacks perform native I/O without VM/SDL entry or guest
suspension. The surrounding player remains owner-thread/synchronous; automatic
launcher wiring and an asynchronous player entry are separate integration work.

`granted_client` удерживается до возврата player. Каждый fetch владеет отдельным
fid и независимо освобождает его при успехе, отказе, EOF или ошибке checksum.
Прерванная загрузка не портит следующее открытие исправного соединения. Сокет,
TLS/authentication и deadlines предоставляет существующий host transport;
стандартный fd-client поддерживает свой native I/O owner и ограниченные RPC.

## Byte-stream schema / Схема byte stream

Definitions: [`protocol.h`](protocol.h), codecs: [`protocol.c`](protocol.c).
Both directions start with a standard S-record tag0 declaration binding the
**dedicated stream-local tag1** to `flashvm.movie`, version1. This does not reserve
tag1 on the terminal's unrelated control stream. Records use the existing CRC and
canonical integer codecs. Fragmentation and coalescing are supported in both
directions; 9P read/write boundaries are not record boundaries.

| Kind byte | Remaining payload / Остальное содержимое |
| --- | --- |
| REQUEST = 1 | Purpose byte CODE=1 or DATA=2; little-endian u16 reference length, u16 base length; counted reference and base bytes, no embedded NUL |
| BEGIN = 2 | Nonempty canonical absolute virtual origin, with no terminating NUL on the wire |
| BYTES = 3 | Nonempty opaque bytes, emitted in bounded chunks |
| END = 4 | One byte: 0 acknowledged success, 1 acquisition failure |

A maximum4095 bytes per reference/base fits the existing DisVM namespace bound.
Each resource gets a new stream/fid: exactly one REQUEST, optional BEGIN/BYTES,
then END. Control declarations can precede the request. Unknown purpose/type/
version, corrupt frames, overlong input and pipelined requests refuse. No request
IDs are needed across independent fids. EOF **before END is always failure**,
even after receiving an entire valid SWF. Resource/sink errors discard staged
bytes before import. No retry through a native file or alternate HTTP adapter.

Два пути ограничены namespace limit; каждая загрузка использует свой fid. Порции
BYTES не означают успех загрузки: импорт допустим только после успешного END.
При подтверждённом prefix с последующим отказом передаётся prefix и END=1;
неопределённые effects, AGAIN и отмена не превращаются в успешный EOF.

[`request.c`](request.c) resolves relative names against the movie's returned
virtual origin, then calls `dvm_resource_plan_intent` with SWF or DATA intent.
Provider-first selection does not reorder the ordinary data stack. The actual
open/read decides access; no permission is obtained from BEGIN, metadata or
purpose. Held source handles close before terminal response or on fid teardown.
All buffers are native and bounded; there are no shared Dis/AS heap pointers.

## Build and acceptance / Сборка и проверки

The native libraries already require explicit `INFERNO_HOST_SUPPORT` and
`INFERNO_VFS_SUPPORT`; the latter selects the FlashVM protocol/header sources as
well as existing VFS/port sources. No FlashVM player binary is needed to build
`styx_movie_service`, `styx_movie_client` and `movie-protocol-test`.

Optional actual-player acceptance requires a separately built **AGG player-jit
FlashVM desktop SDK**, SDL2 development files, C99 native and C++98 player tools.
Set `INFERNO_MOVIE_PLAYER_BUILD` explicitly; it names compiled SDK archives, not
an inferred sibling build directory. From the repository root:

```sh
cmake -S disVM/native -B build/native \
  -DINFERNO_HOST_SUPPORT="$PWD/tools/limbo-bootstrap" \
  -DINFERNO_VFS_SUPPORT="$PWD" \
  -DINFERNO_MOVIE_PLAYER_BUILD="$PWD/build/flashvm-managed"
cmake --build build/native --target movie-server movie-player movie-protocol-test --parallel
ctest --test-dir build/native -R '^managed-movie-' --output-on-failure
```

For SDK setup/build see the [managed player guide](../../../flashVM/doc/MANAGED_SOURCE.md#validation).
Supply SDL2 through its normal package or an explicit CMAKE_PREFIX_PATH.
`tests/wire.py` uses the explicitly selected SDK's independent raw SWF oracles.

Wire acceptance runs **22 scenarios**: interpreter/JIT; conflicting code/data
providers; explicit denial with a valid local SWF present; full valid SWF prefix
followed by source denial; confined symlink refusal; long fragmented requests/
origins; sink refusal followed by reuse of the same connection; bad CRC, early EOF,
truncation, missing END and coalesced replies. Negotiated msize256 forces actual
9P fragmentation. Script verifies decoded variables, and JIT cases require real
native entries. The protocol unit checks independent opens, one-byte framing,
coalesced declarations, zero reads, bad CRC, unbound requests and size/purpose
refusal. Both tests pass normally and under ASan/UBSan (LeakSanitizer disabled).

Проверки выполняют настоящий player и исходный C-сервер libstyx по TCP; здесь нет
mock-функции, возвращающей SWF вместо сети. SDL dummy-video проверяет исполнение
и загрузку, а не физический дисплей. Поддержанная платформа этого native SDK —
Linux64. Автоматический запуск FlashVM из guest init, привязка служебного stream
к Terminal Session, общие файловые записи/сокеты, input filtering и динамическое
добавление media/bulk channels этим изменением не реализуются.
