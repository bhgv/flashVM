# DisVM terminal launch CLI / Команда запуска DisVM через терминал

The optional Linux CLI runs one authorized Dis job over a connected Styx stream.
The caller connects and authenticates the session before delegating the socket.
The local operator supplies the logical launch label, exact code export paths,
and a trusted local bootstrap. The peer supplies startup bytes on the selected
route. The process returns the DisVM exit code or a launch admission error.

Необязательная команда Linux запускает одну Dis-задачу через выданный сокет
Styx. Вызывающий устанавливает соединение и проверяет собеседника до передачи
сокета. Оператор задаёт имя запуска, разрешённые пути кода и доверенный локальный
bootstrap. Терминал передаёт стартовые байты через согласованный маршрут.

## Local companion process

`disvm --styx-local-pair PROGRAM [PEER OPTIONS] -- [MOUNT OPTIONS] -- [VM OPTIONS] BOOTSTRAP.dis`
runs the same bidirectional mount as `--styx-mount-fd` over a private Unix
socket pair. `PROGRAM` is an explicit executable path. It receives `--fd FD`
followed by `PEER OPTIONS`. DisVM owns and reaps that process. Exit status
comes from DisVM, or from its companion if DisVM completed successfully.

An optional `--assets ABSOLUTE_DIRECTORY` after `--at` installs a read-only
`/ide-assets` mount and selects that directory for initial relative module
imports. The workspace remains the process's current directory. The paired
`--document PATH` or `--new PATH` option passes a guest VFS path to the
application through the existing Unibus launch binding. `--new` asks the
application to initialize its buffer without reading the file first.

[Installed desktop example](../../ide/DESKTOP-PAIRED.md)

[Typed mixed UniBus exchange](STYX-CHANNEL.md) uses the bidirectional mount session already established by the CLI.

## TCP endpoints

The hosted Linux64 build can open its own carrier. Use
`--styx-mount-tcp HOST PORT` in place of `--styx-mount-fd FD`,
`--styx-serve-tcp HOST PORT` in place of `--styx-serve-fd FD`, or
`--styx-library-serve-tcp HOST PORT` in place of `--styx-library-serve-fd FD`.
Keep the remaining arguments of the corresponding command. A mounted Dis
program sees the same live VFS and delegated StyxRPC context.

```sh
disvm --styx-serve-tcp 127.0.0.1 7474 --persistent --execute-as job.dis job.dis -- --native-spawn owner.dis
disvm --styx-mount-tcp 127.0.0.1 7474 --at /peer -- --native-spawn caller.dis
```

Run the listener first. `HOST` is its local bind address or the dialer's peer
address. The port is decimal. Listen on port `0` to select an ephemeral port
and read `styx listening PORT` from the first stdout line. Each listener
accepts one transport connection. `--persistent` permits multiple job launches
on that connection. Select `--raw` on a mount only for a server that speaks
ordinary Styx without the bidirectional carrier. TCP is plaintext and does
not authenticate the peer. Use loopback, a trusted network, or a separately
protected stream delegated through the FD command.

[FlashVM player commands](../../flashVM/doc/STYX_RPC_PLAYER.md#start-over-tcp) ·
[Carrier API](../../lib/styx/docs/reference/phytcpdial.md)

## Build / Сборка

From the repository root, build the native SDK, then configure the runtime
against the SDK build from the same checkout.

Из корня репозитория соберите SDK, затем укажите каталог его сборки при
настройке runtime. Используйте один и тот же checkout для обеих сборок.

```sh
cmake -S disVM/native -B build/native-sdk \
  -DINFERNO_HOST_SUPPORT="$PWD/tools/limbo-bootstrap" -DINFERNO_VFS_SUPPORT="$PWD"
cmake --build build/native-sdk --target terminal-ordered-files --parallel
cmake -S disVM/runtime -B build/disvm-terminal \
  -DDVM_RUNTIME_TESTS=OFF -DDVM_TERMINAL_LAUNCH_SDK="$PWD/build/native-sdk"
cmake --build build/disvm-terminal --target disvm --parallel
```

## Invoke / Запуск

```sh
build/disvm-terminal/disvm --terminal-launch-fd 3 my-app app.dis dep.dis \
  -- --jit /trusted/bootstrap.dis
```

| Input / Вход | Meaning / Значение |
| --- | --- |
| FD | Connected stream socket, descriptor 3 or greater / Подключённый потоковый сокет, номер от 3 |
| Label | Logical name read through `Launchcontext.name()` / Логическое имя запуска |
| Code file | Exact path in the granted Styx root, exposed as `/app.dis` / Точный путь кода в выданном корне |
| Dependency | Exact path exposed as `/dep.dis`, or `-` for none / Путь зависимости или `-` |
| After `--` | Ordinary VM options and local bootstrap path / Обычные опции VM и путь локального bootstrap |

Optional arguments go before the separator.

Дополнительные параметры указываются перед `--`.

| Option / Параметр | Default / По умолчанию | Effect / Действие |
| --- | --- | --- |
| `--request PATH TAG` | `launch 17` | Read the request from a separate Styx file under a nonzero negotiated tag / Читать запрос из отдельного файла Styx под указанным тегом |
| `--records RX TX` | `.unibus .unibus` | Choose the ordered Unibus input and output files / Выбрать входной и выходной файлы ordered Unibus |
| `--lane TAG` | `71` | Select the nonzero ordered TX lane / Задать ненулевой тег исходящего ordered-канала |
| `--timeout-ms MS` | `5000` | Set each connection and mailbox wait budget, 1–120000 ms / Задать срок отдельного ожидания подключения и запроса |
| `--grant PATH TAG MODE` | `/zero 0 rw`, `/seven 7 rw`, `/nine 9 r` | First explicit grant replaces all defaults. Repeat up to 32 times with distinct paths/tags and mode `r`, `w` or `rw` / Первая выдача заменяет все значения по умолчанию. До 32 уникальных путей и тегов |
| `--fd-grant PATH NODE MODE` | None / Нет | Expose one terminal file at an absolute guest path. Up to 32 distinct aliases and relative source nodes with mode `r`, `w` or `rw` / Выдать файл терминала по абсолютному пути гостя. До 32 разных путей и относительных исходных имён |
| `--inherit-fd NUMBER MODE` | Automatic / Автоматически | Optional explicit selection from the terminal's `/.fd`. Mode `r`, `w` or `rw`. One entry per number from 0 to 31 / Необязательный явный выбор из `/.fd` терминала в заданном режиме. Номера от 0 до 31 |

```sh
build/disvm-terminal/disvm --terminal-launch-fd 3 my-app app.dis dep.dis \
  --fd-grant /input held-input r --fd-grant /output held-output w \
  -- --jit /trusted/bootstrap.dis
```

The launcher takes an exact file fid for each `NODE` from the accepted terminal
root. It checks file metadata before starting the guest. The guest obtains its
local `Sys->FD` by opening `PATH`. When the peer publishes an already-open
source with [`styxheldexport`](../native/vfs/HELD-EXPORT.md), that open takes the
retained cursor exactly once. The peer controls the lifetime and access to its
published node. These aliases are configured by the host before launch. Request
bytes carry application arguments.

Лаунчер закрепляет точный fid каждого `NODE` в принятом корне терминала и
проверяет метаданные до запуска. Гость открывает `PATH` и получает локальный
`Sys->FD`. Если терминал опубликовал открытый файл через
[`styxheldexport`](../native/vfs/HELD-EXPORT.ru.md), гость один раз получает
его текущую позицию. Терминал управляет доступом к опубликованному узлу.
Имена выдачи задаёт хост до запуска, а байты запроса содержат аргументы приложения.

### Numbered descriptors / Номерные дескрипторы

```sh
build/disvm-terminal/disvm --terminal-launch-fd 3 my-app app.dis dep.dis \
  -- --jit /trusted/bootstrap.dis
```

The publisher supplies open files with [`styxheldfdmap`](../native/vfs/HELD-FD-MAP.md).
The runtime first discovers `/.fd` in the accepted execution owner's root. If
the owner's directory is absent, it checks the accepted terminal root. The
first available directory supplies the complete descriptor set. Its file
metadata declares the read/write modes, and each open is checked by the VFS
provider before `init`. In Limbo, `sys->fildes(3)` has
`fd == 3`. Repeated calls and spawned children share the same open cursor.
Ordinary guest opens skip inherited numbers. `sys->print` uses inherited 1 when
present. Unspecified 0–2 use the host's standard streams; other unspecified
numbers return nil from `fildes`. The modes grant read, write, or both without
truncation. If an open fails, the guest does not start. Any earlier opens in
that batch are closed and a consumed one-use grant stays consumed.

Владелец публикует открытые файлы через [`styxheldfdmap`](../native/vfs/HELD-FD-MAP.ru.md).
Runtime ищет каталог `/.fd` в принятом корне владельца исполнения. Если
каталога там нет, используется принятый корень терминала. Первый найденный
каталог задаёт весь набор дескрипторов. Режимы чтения и записи берутся из
метаданных файлов, а каждое открытие проверяет VFS-провайдер до `init`.
В Limbo у результата `sys->fildes(3)` поле
`fd` равно 3. Повторные вызовы и дочерние процессы используют общую позицию.
Обычные открытия гостя обходят унаследованные номера. `sys->print` пишет в
выданный FD 1. Для непереданных 0–2 используются стандартные потоки хоста,
для остальных номеров `fildes` возвращает nil. Режимы задают чтение, запись
либо оба права без усечения. Если открыть один из файлов не удалось, гость
не запускается. Полученные ранее в этой попытке файлы закрываются, а
поглощённая одноразовая выдача остаётся поглощённой.

The default S-record endpoint is `.unibus` in the granted Styx root. For an older peer exposing `records`, select `--records records records`.

По умолчанию S-записи идут через `.unibus` выданного корня Styx. Для старого узла `records` задайте `--records records records`.

The peer serves P9MX channel 0 and declares the selected request tag with type
`disvm.launch`, version 1. It sends one launch record on that tag. The request
file and the Unibus input file must be distinct. Launch context and startup
acknowledgement are handled by the bootstrap and guest protocol.

Терминал обслуживает P9MX channel 0, объявляет выбранный тег запроса с типом
`disvm.launch` версии 1 и отправляет одну запись запуска. Файл запроса отличается
от входного файла Unibus. Подтверждение запуска отправляет гость по протоколу
своего bootstrap.

The optional SDK CTest `terminal-ordered-files-cli` uses an independent original
Styx server over TCP loopback. Set `INFERNO_DISVM_TERMINAL_CLI` together with the
existing guest runtime and compiler settings to select the CLI binary.

Для проверки укажите `INFERNO_DISVM_TERMINAL_CLI` вместе с путями runtime и
компилятора. CTest `terminal-ordered-files-cli` запустит отдельный Styx-сервер
через TCP loopback.

### Reverse Unibus completion / Встречный результат Unibus

The launcher automatically serves a write-only `.unibus` on P9MX channel 1.
The authorized peer can write one completion S-record with tag 17. The guest
reads that record through `Sys->open("/reverse-exit", Sys->OREAD)` and
`Sys->read`. The reserved alias belongs to the launcher. Before delivery,
a nonempty read returns an error indicating that data is pending.

Лаунчер автоматически публикует `.unibus` для записи на канале 1 P9MX.
Выданная сторона может отправить одну S-запись завершения с тегом 17.
Гость открывает `/reverse-exit` через `Sys->open` в режиме чтения и читает
запись через `Sys->read`. Имя зарезервировано лаунчером. До доставки
непустое чтение сообщает, что запись ещё не поступила.

[Native inbox](../native/terminal/UNIBUS-INBOX.md) · [Приёмник](../native/terminal/UNIBUS-INBOX.ru.md)

## Bidirectional mount for a local program

The same SDK-enabled build supports a delegated connected stream for a
local bootstrap that loads libraries from a peer.

```sh
disvm --styx-mount-fd 3 -- --no-jit caller.dis
```

The default mount point is `/peer`. Pass `--at /transport` before `--fd3` or
`--` to choose a canonical absolute guest path. The host directory does
not need an entry at the mount point. The multiplexed carrier uses channel 0 for
the forward Styx mount and channel 1 for the caller's virtual context.
The peer mounts channel 1 and supplies that root to its scoped function
handler. An ordinary typed `load` and function call then publish the active
caller automatically. The peer is selected by the delegated connection.

The [Limbo example](../examples/remote-scalar/styx-cli.b) and
`terminal-caller-mount-cli` check the command with two OS processes and
both VM execution modes. Configure the native SDK test build with
`INFERNO_DISVM_TERMINAL_CLI` pointing to the SDK-enabled DisVM executable.

For peers without the built-in launch declaration, configure an explicit
program entry, writable request file and S-record tag:

```sh
disvm --styx-mount-fd 3 --launch app.dis submit 17 -- caller.dis
```

The entry and request paths are relative to the attached peer. The mount
pins both nodes. The guest calls `Vfsnative.run` or `Vfsnative.runargs` on
`/peer/app.dis` or one of its bind aliases. The provider must serve the
selected request file and accept the configured record tag. A positive
result confirms submission to the provider. This profile carries raw
startup bytes.

The [launch submission example](../examples/remote-scalar/styx-run.b)
verifies exact binary arguments through a bind alias and rejection of an
unadvertised neighbour. It runs in the same `terminal-caller-mount-cli`
test.

### Run both sides

Give the owner and caller opposite ends of a connected stream. On the
owner, launch the one-shot service with the local Dis image it will run:

```sh
disvm --styx-serve-fd 3 -- owner.dis
```

The service exports `implementation.dis` and accepts one S-record with
tag 17 at `.unibus`. Start the caller with its descriptor of the same
connection:

```sh
disvm --styx-mount-fd 3 -- caller.dis
```

The mount recognizes the owner's read-only `.interface` declaration,
then pins the program and request files automatically. An explicit
`--launch` or `--launch-context` takes precedence over discovery.

The owner runs its configured image and exposes the accepted bytes to
`Launchcontext.arguments()`. The complete record must fit the one-shot
64-byte inbox. This launch profile carries startup bytes and does not
select the caller's process context. The [owner example](../examples/remote-scalar/styx-owner.b)
checks an embedded NUL. The acceptance test starts two separate DisVM
processes with these commands.

### Launch with the live caller context

Use the context profile on both ends of the same delegated connection:

```sh
disvm --styx-serve-fd 3 --context -- owner.dis
disvm --styx-mount-fd 3 -- caller.dis
```

The Limbo caller uses the same `Vfsnative.runargs` call. DisVM publishes
its virtual `.prog/<pid>` before sending the request. The owner mounts
the reverse channel, pins the PID directory and adopts its available
context before `init`. The scoped send holds the export until
completion, loss of the carrier or cancellation.

The two-process example reads the caller PID and consumes redirected
stdin from the owner with `Sys->fildes(0)`. It also writes through the
caller's redirected stdout. The 64-byte inbox limit includes the S-record
and process headers.

## Owner-side Styx library mount

Use `disvm --styx-serve-fd FD --context --library-fd LIBFD -- OWNER.dis`
to give the launched Dis a second, already connected Styx stream.
`LIBFD` is a separate delegated stream FD of at least 3. It carries a
plain Styx session. The owner mounts it at `/libraries` before accepting
the launch. An unavailable library mount makes the server refuse startup.
The library is resolved by ordinary Dis `load` in the owner's namespace.
See the [three-peer example](../examples/remote-scalar/STYX.md#three-peer-launch).

### Serve repeated standalone launches

```sh
disvm --styx-serve-fd 3 --persistent -- owner.dis
disvm --styx-mount-fd 3 -- caller.dis
```

`--persistent` implies the caller-context profile. The owner reads the
configured Dis image at startup, publishes that same immutable image and
waits on the connected session. Each bound-path `run` or `runargs` starts
its own child with the caller's context and reads its own completion.
Separate callers can run concurrently. The listener is built into DisVM.
Use `--library-fd` or `--library-bidir-fd` to mount dependencies on the
owner before accepting requests. Without `--persistent`, the standalone command still exits
after the single configured run.

The [standalone client](../examples/remote-scalar/styx-persistent-cli.b)
checks two consecutive launches and two overlapping launches with
different outcomes. The three-peer test also runs this mode with
inherited descriptors and separately mounted libraries.

In persistent mode, cancelling the caller's wait closes the launch fid.
The owner drops a queued request or interrupts its registered child at
a VM termination checkpoint. A definitive process result is unavailable
after the caller closes the fid. Other launches keep their own fids and
the same session remains available.

### Launch result and an extra descriptor

For a scoped launch, `Vfsnative.run` and `runargs` write to a read/write
`.unibus` fid and read the result on that same fid. The calls return
`1` for success, `-2` for a reported execution failure and `-1` if the
outcome is unknown. A fid can read the result after its own request was
accepted. The legacy read-only `.exit` file remains available. The
one-shot owner waits up to five seconds for disconnect after reporting
completion.

At client bootstrap, `--styx-mount-fd FD --fd3 EXTRA -- APP.dis`
exposes a separately delegated regular file as guest FD 3 alongside
the standard streams. It shares the supplied open file's access mode
and position. The guest launch command still supplies only its VFS path.

Add `--library-at /remote-libs` after `--library-fd LIBFD` to choose the
owner's VFS mount point. The default is `/libraries`. Use a canonical
absolute path outside the reserved `/.prog`, `/.fd`, `/.vfs` and
`/.var` trees. The three-peer example uses `/remote-libs` and binds
its `mounted` directory into the application's library union.

Repeat the `--library-fd FD --library-at PATH` pair for up to eight
library mounts. Each FD and mount path must be distinct. Nested mount
paths are rejected before any connection is opened. All mounts must
succeed before the owner accepts a launch.

Use `--library-bidir-fd LIBFD` for a library peer running
`--styx-library-serve-fd`. It mounts the same forward Styx tree and
starts a reverse Styx server for the launching DisVM. Compiled calls
can use the caller's `.prog/.self`, including granted `.vfs` files and
inherited FDs. Both descriptor options accept `--library-at PATH`.
The caller supplies an already connected transport. Endpoints can
reside on separate hosts.

The bidirectional mount also discovers executables. A peer started with
`--styx-library-serve-fd FD --execute IMAGE.dis` advertises `implementation.dis` and
may add named images under `programs/`. With the mount at
`/remote-libs`, use `Vfsnative->run("/remote-libs/implementation.dis")`
or bind an advertised file to a local path before calling `run`.
A program started over Styx can use the same VFS path to launch a
dependency on the next peer. The next launch receives that process's
current descriptor views and VFS grants. An explicit `/.var` mount selects
the outgoing variable tree. In its absence the incoming launch grant and its
access modes pass through to the next peer. The provider sees variables at
`/.prog/.self/.var`. Both launches keep their own fids through completion or
cancellation. See the
[three-peer example](../examples/remote-scalar/STYX.md#three-peer-launch).

Different descriptor numbers that alias the same socket are rejected before any Styx handshake.

### Publish additional executables on the same mount

Repeat `--execute-as NAME.dis IMAGE.dis` after `--persistent` and before
any library carrier options. Each name is a unique public basename of at most
31 ASCII characters. Start it with a letter or digit, use letters, digits,
dots, underscores or hyphens, and end it with `.dis`. The listener accepts
up to 16 additional images and retains a snapshot of each for the session.

```sh
disvm --styx-serve-fd 3 --persistent --execute-as second.dis other.dis -- owner.dis
```

An automatic mount discovers the extra paths through `.programs`. With the
default mount point, the existing entry is `/peer/implementation.dis` and the
additional entry is `/peer/programs/second.dis`. Pass either path to
`Vfsnative->run` or bind it to a local alias before calling `run`. The named
request uses `programs/.second.dis` on the existing carrier. Its open fid
selects the image and carries its own completion, including errors and
cancellation. The launch record retains the same caller context and argument
format.

The combined library server also accepts `--execute-as` after its
`--execute PROGRAM.dis` option and before the first `--export`. Its
library exports remain under `mounted/`. The
[client example](../examples/remote-scalar/styx-launch-client.b) exercises
direct and bound paths with the
[second executable](../examples/remote-scalar/styx-named-launch-owner.b).


## Serve several loaded libraries over one Styx session

Run the provider with one connected stream delegated by the host. Repeat
`--export PUBLIC.dis OWNER.dis NAME` for each export the provider publishes.

```sh
disvm --styx-library-serve-fd 3 \
  --export first.dis first-owner.dis update \
  --export second.dis second-owner.dis update \
  -- scalarowner.dis
```

`PUBLIC.dis` is the name under `/peer/mounted` on the caller.
`OWNER.dis` is the provider's implementation path, and `NAME` is the
export name in that image. Compile the owner implementations and caller
with `limbo -D R` for scoped export type metadata. `scalarowner.dis` is
the dispatcher built from [scalarowner.b](../limbo/scalarowner.b). Arguments
after `--` are ordinary VM options followed by the dispatcher image.

The caller starts `disvm --styx-mount-fd 3 --at /transport -- caller.dis`
on its end of the delegated stream. Create a local `libraries` directory
in the caller working directory. The [example caller](../examples/remote-scalar/styx-library-cli.b)
appends `/transport/mounted` to `/libraries` with `Sys->bind` and `MAFTER`,
then uses ordinary Limbo `load` for both public paths. The [integration test](../native/terminal/tests/scalar/mount_cli.py)
compiles the images, connects the two processes, checks ref copy-back and
compares the provider PIDs. After cancellation, the same caller PID gets
a new executor in the other library. A missing export fails provider startup
and reports the implementation and export name. Disconnecting the caller drains the provider's
export inboxes and retires its idle workers.


### Additional numbered FDs

Repeat `--fd N HOSTFD` before `--` to delegate an open regular file to a
guest descriptor number from 3 through 31. `--fd3 HOSTFD` is shorthand
for number 3. The open descriptor determines read and write access.
Two guest numbers can refer to the same host FD and share its read and
write position. Guest numbers 0, 1 and 2 come from the standard streams.
The [launch example](../examples/remote-scalar/styx-chain-owner.b) checks
successive reads through two numbers and a write through one of them.

Files already opened by the caller with `Sys->open` or `Sys->create`
are published from its file group without extra command-line options.
The new virtual numbered files preserve the original access and cursor.
Explicit numbers supplied by `--fd` reserve those numbers in the caller.
Any guest `/fd` mount remains independent of this publication.

A scoped launch also exposes the adopted numbers under `/.prog/.self/.fd`.
Opening one of these files uses the same slot, cursor and mode as
`Sys->fildes(N)`. An existing caller `/env` directory is visible under
`/.vfs/env` when it is part of the caller's VFS grant.

### Serve a program and libraries together

A library owner can mount its own dependencies using
`--library-bidir-fd FD [--library-at PATH]`. Put this option before
the final `--`. The dependencies are available to the libraries'
ordinary `load` instructions. A nested scoped call receives the incoming
caller grants for `.fd`, `.vfs`, and `.var`, subject to explicit local
mounts. See the [three-VM example](../examples/remote-scalar/STYX.md#run-on-separate-hosts).

Place `--execute PROGRAM.dis` before the export table:

```sh
disvm --styx-library-serve-fd 3 --execute launch.dis \
  --export first.dis first-owner.dis update \
  --export second.dis second-owner.dis update -- scalarowner.dis
```

The same Styx mount publishes the program at `implementation.dis` and
libraries under `mounted`. The caller can bind the program to another VFS
path and submit it with `Vfsnative.run`. Each call runs the configured
image in its own child Dis process with its caller context and receives its
own result on the opened fid. Different callers can run concurrently and
the same caller can run again after completion. The host queues up to 64
pending launch records. Each record, including the selector and S-record
header, fits in 64 bytes. Library calls stay available on the same session.
The standalone command can use the same queue with `--persistent`.

The [stock CLI test](../native/terminal/tests/scalar/mount_cli.py) runs
the [example program](../examples/remote-scalar/styx-shared-launch-owner.b),
checks the result and makes library calls on the same mount.
