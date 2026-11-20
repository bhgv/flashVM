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
| `--records RX TX` | `records records` | Choose the ordered Unibus input and output files / Выбрать входной и выходной файлы ordered Unibus |
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
