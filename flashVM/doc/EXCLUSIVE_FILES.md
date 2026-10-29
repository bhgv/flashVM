# Exclusive new-file output — v0.1672

The private shared file port accepts `wx` in addition to `r`, `w` and `rw`.
[fvm_port_file_open](../ports/file_port.h#L24) allocates its handle/path before opening
and returns an owned write-only binary stream or NULL/error. The selected native
adapter uses POSIX O_CREAT|O_EXCL or Windows _O_CREAT|_O_EXCL, without a stat/open
race. Existing regular files, directories, hardlink names and symlinks (including
dangling symlinks) are rejected without truncation or replacement. Ordinary w/rw
semantics remain unchanged; this does not alter the public FlashVM C ABI.

The path is borrowed UTF-8 and must pass the existing ordinary-path validation;
provider/URI paths do not silently fall back to native stdio. The caller closes
and externally serializes the handle. OS operations may block. A failure after
creation can leave a new empty/partial file; no atomic content publication,
rollback or fsync/durability is promised. The primitive is not a filesystem sandbox.

The Dis runtime maps OWRITE|OEXCL onto this mode with permission 0666 filtered by
the OS umask. [IDE export](../../ide/ARTIFACT-EXPORT.md) always uses exclusive binary
creation. It never changes the ordinary source-save overwrite policy.

The native Linux ASan/UBSan fixture exercises 256 byte values, existing and
symlink preservation, and eight barrier-released processes (one creator wins).
Windows/macOS/Android implementations are adapted source, not native acceptance.
No complete Flash compatibility or physical platform roadmap stage is closed.

## Русский

Режим wx создаёт только новый бинарный файл. Проверка эксклюзивности выполняется
самим OS-open, поэтому между проверкой существования и созданием нет гонки.
Существующие файлы/каталоги/симлинки сохраняются. Владельцы выделяются до открытия.
Более поздняя ошибка может оставить частичный новый файл: атомарная публикация
содержимого, откат и fsync не реализованы. Старые режимы w/rw не меняются.
Нативная проверка выполнена на Linux; физические остальные платформы не заявлены.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_port_file_open` | function | [ports/file_port.c:44](../ports/file_port.c#L44) |
<!-- source-links:end -->
