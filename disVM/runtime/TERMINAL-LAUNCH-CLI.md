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
