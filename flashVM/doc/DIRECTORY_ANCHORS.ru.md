# Portable anchors каталогов

C89 API [anchor_port.h](../ports/anchor_port.h) не раскрывает OS types.
POSIX держит directory descriptor: relative операции не меняют его cursor и не
переоткрывают display path. Это граница портирования, не sandbox и не полный VFS.

| Функция | Аргументы, результат, владение |
| --- | --- |
| [`fvm_port_anchor_open`](../ports/anchor_port.h#L25) | Заимствует непустой native path и адрес output. Открывает читаемый каталог; успех передаёт anchor, отказ очищает output. |
| [`fvm_port_anchor_open_at`](../ports/anchor_port.h#L32) | Заимствует живой parent и непустой relative path каталога; передаёт независимый anchor. Absolute child запрещён. |
| [`fvm_port_anchor_close`](../ports/anchor_port.h#L37) | Поглощает anchor после завершения всех borrowers; NULL допустим. |
| [`fvm_port_anchor_info_at`](../ports/anchor_port.h#L44) | Заимствует parent/path/output; копирует metadata при успехе, не меняет cursor. |
| [`fvm_port_file_open_at`](../ports/anchor_port.h#L63) | Заимствует parent/path/mode/output. Обычные r/w/wx/rw; передаёт независимый file, закрываемый обычным file API. |
| [`fvm_port_directory_open_at`](../ports/anchor_port.h#L70) | Заимствует parent/path/output; передаёт свежий bounded cursor, закрываемый обычным directory API. Два cursors не разделяют offset. |

Все вызовы могут блокировать, не вызывают VM/GC. Parent удерживается caller до
конца операции; close не является concurrent cancel. Независимые результаты
работают параллельно; один file/cursor требует обычной внешней синхронизации.
Для parent не нужен seek-lock. Relative path допускает native dot/dot-dot и
symlinks; absolute child отвергается. Сохранены существующие bounds путей/имён.

Неверные pointers/path/mode дают INVALID_ARGUMENT, allocation — OUT_OF_MEMORY,
native failure — ERROR; непредставимое имя/отсутствующий backend — UNSUPPORTED.
Отказ очищает handle output; metadata меняется только при успехе. Сохраняется
errno для native ошибок. Identity каталога переживает rename, children могут
меняться: транзакционный снимок child names не обещается.

State/path выделяются до destructive open. POSIX откладывает truncate до успеха
fdopen. Поздний отказ закрывает настоящий FD и сохраняет существующий файл.
Создание нового файла может оставить пустой файл при последующем отказе.
Rollback через unlink потенциально подменённого pathname не выполняется.
Действуют host permissions и umask.

POSIX использует openat/fstatat и отдельно открытые fdopendir с close-on-exec;
каталоги требуют read permission. Windows явно возвращает UNSUPPORTED новых
операций, без имитации anchors конкатенацией путей. ABI и обычные file/directory
операции сохранены.

Linux x64: настоящий rename с подменой старого имени, metadata, cursors,
восемь OS owners/1024 чтения, exclusive create и поздний отказ stdio. DisVM
проверяет удержание через FORKNS, rebind/unmount, imports и retirement.
macOS/Android POSIX и будущий Windows backend требуют native приёмки. Этапы Flash
не закрываются. Внешних runtime библиотек и инструментов не добавлено.

[DisVM](../../disVM/runtime/DIRECTORY-ANCHORS.ru.md),
[English](DIRECTORY_ANCHORS.md).

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_port_anchor_close` | function | [ports/anchor_port.h:37](../ports/anchor_port.h#L37) |
| `fvm_port_anchor_info_at` | function | [ports/anchor_port.h:44](../ports/anchor_port.h#L44) |
| `fvm_port_anchor_open` | function | [ports/anchor_port.h:25](../ports/anchor_port.h#L25) |
| `fvm_port_anchor_open_at` | function | [ports/anchor_port.h:32](../ports/anchor_port.h#L32) |
| `fvm_port_directory_open_at` | function | [ports/anchor_port.h:70](../ports/anchor_port.h#L70) |
| `fvm_port_file_open_at` | function | [ports/anchor_port.h:63](../ports/anchor_port.h#L63) |
<!-- source-links:end -->
