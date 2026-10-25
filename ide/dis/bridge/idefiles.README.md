# Idefiles: bounded binary resource windows

[Protocol](idefiles.m) · [Limbo implementation](idefiles.b) ·
[terminal IDE](../examples/editor-console/README.md) ·
[native runtime](../../../disVM/runtime/README.md) ·
[real runtime regression](../../../disVM/runtime/test_file_windows.py)

Idefiles is a read-only Dis service for inspecting compiled resources or loading
bounded source/debugger views. It uses genuine Sys open/seek/readn operations;
there is no host implementation of IDE policy. The current terminal editor uses
it for `inspect PATH`, displaying the size and first sixteen bytes as hex.

`start(sys)` borrows an initialized Sys module, spawns one Dis owner and returns
its blocking request endpoint; nil Sys returns nil. Every admitted request needs
a nonnil private reply channel. The client must keep request values unchanged
until it consumes the response. A missing reply endpoint is ignored. Responses
echo the caller's nonnegative ID; use distinct IDs when multiplexing callers.

| Request | Contract |
| --- | --- |
| `Read` | Open an ordinary read-only native path, measure its extent, select a window, read at most `count` bytes, close and reply. |
| `Shutdown` | Reply `Ok`, then terminate the owner. The caller must consume this final reply. |

Read accepts a valid nonempty Unicode path of at most 4096 UTF-8 bytes, no NUL,
a count from zero through 65536, a signed `big` offset and a Sys origin.
`SEEKSTART` and `SEEKRELA` both start from zero because each request opens a fresh
handle; `SEEKEND` starts from the measured current end. Negative end-relative
offsets are supported if they resolve to a nonnegative position. Reading beyond
EOF returns `Ok` with an empty byte array, preserving the selected position.

`Reply.size` is the extent measured before reading; `position` is the actual
selected byte offset; `bytes` is the exact short/full read, including embedded
NUL. The owner retains neither bytes nor file descriptors after sending the
reply, so clients may mutate received arrays. The file is closed before the
reply rendezvous: abandoning a reply can still block the process, but cannot
leave that file open. `Invalid` rejects malformed protocol values before open;
`Failed` reports open/seek/read failure. Error replies contain nil bytes and
size/position -1. Diagnostics are plain fixed strings, not executable content.

Input is externally mutable: measuring and reading are separate operations and
do not promise an atomic snapshot against another writer. This service reads
ordinary host paths with the host's permissions; it is not a virtual namespace,
capability sandbox, file watcher or Styx client. Native I/O in the current runner
can block its single OS owner. Cancellation and timeout are not yet part of the
protocol. Dis request processes use the existing scheduler and blocking channels.

The standalone `ide/dis` CMake project builds target `ide_file_windows` when
explicit `LIMBO_EXECUTABLE` and `LIMBO_MODULE_DIR` (containing sys.m) are supplied.
It has no dependency on Uibridge, another source checkout or a running emu. Its
ABI follows the explicitly selected compiler; keep those products separate from
NC64 runtime modules. The root `ide` package includes this classic module too.

Root `python3 build_menu.py`, `all` (or `disvm-runtime`) builds and installs the
module beside the matching NC64 runtime, `.sbl` debug information, source/interface
and this guide. Menu launch 5 opens the terminal editor. Build engine selection
and post-build tests remain available from the interactive menu.

## По-русски

Сервис читает ограниченные бинарные окна для просмотра ресурсов, исходников и
будущих панелей отладчика. Вся логика находится в Limbo, в отдельном процессе Dis;
доступ выполняется через типизированные блокирующие каналы. Один запрос — один
ответ с тем же ID. Перед ответом файл закрывается, массив результата не хранится
в сервисе. Клиент обязан принять ответ, в том числе подтверждение Shutdown.

Размер окна — до 64 КиБ, путь — до 4096 байт UTF-8, смещения — `big` с проверкой
диапазона нативного порта. Разрешены отрицательные смещения от конца файла, но
итоговая позиция не может быть отрицательной. EOF — успешный пустой результат.
Ошибки не выдают частичные данные за успешные. Измерение и чтение не образуют
атомарный снимок при внешнем изменении файла; это не полная виртуальная ФС Inferno.

В меню сборки выберите `all` либо `disvm-runtime`, после сборки пункт 5. Команда
`inspect editor-console.dis` выводит размер и первые 16 байт, не меняя открытый
документ. Модуль устанавливается вместе с интерфейсом, исходником и debug info.
