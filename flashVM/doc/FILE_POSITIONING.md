# Shared ordinary-file positioning / Позиционирование файлов

[Dis runtime](../../disVM/runtime/README.md) ·
[IDE binary windows](../../ide/dis/bridge/idefiles.README.md) ·
[Declaration](../ports/file_port.h) · [Implementation](../ports/file_port.c) ·
[Native regression](../../disVM/runtime/test_file_seek.c)

`fvm_port_file_seek(file, offset, origin, position)` moves the active cursor of
an existing read-only or write-only [`FVM_PortFile`](../ports/file_port.c#L26). The caller borrows the handle,
provides writable `long` result storage, and serializes calls with reads/writes
and close. This call may block on native stdio operations; it is not an async
channel request. It allocates no handle and never closes the borrowed file.

| Parameter | Contract |
| --- | --- |
| `file` | Live borrowed file from [`fvm_port_file_open`](../ports/file_port.c#L44); nil is invalid. |
| `offset` | Signed native `long` byte displacement; the resulting position must be nonnegative and representable. |
| `origin` | [`FVM_FILE_SEEK_START`](../ports/file_port.h#L51) (zero), [`FVM_FILE_SEEK_CURRENT`](../ports/file_port.h#L52) (active logical cursor), or [`FVM_FILE_SEEK_END`](../ports/file_port.h#L53) (measured extent). |
| `position` | Required output pointer; receives the selected byte position only on success. |

[`FVM_OK`](../include/flashvm/flashvm.h#L44) means the native seek succeeded and the active logical cursor moved.
A read-only handle updates its read cursor; a write-only handle updates its write
cursor. Reading beyond EOF returns zero through the normal read API. A later
positioned write beyond EOF can extend the file with a native sparse/zero gap.
No transactional write, disk quota or durability guarantee is introduced.

[`FVM_INVALID_ARGUMENT`](../include/flashvm/flashvm.h#L47) covers nil arguments, unknown origins, negative resulting
positions and checked arithmetic overflow. [`FVM_UNSUPPORTED`](../include/flashvm/flashvm.h#L48) rejects the existing
`rw` append-channel mode: its read cursor and append-write cursor stay independent.
[`FVM_ERROR`](../include/flashvm/flashvm.h#L45) reports a poisoned write handle, failed extent query or native seek.
Every error preserves both logical cursors and the output value. The native
stream position is an implementation detail, repositioned by ordinary reads/writes.

The adapter currently uses native `long`/`fseek`, not a newly widened portable ABI.
Linux AMD64 supports positions above 2 GiB; a narrower native-long target cannot
accept such positions. Dis checks `big` offset range before narrowing. This
series executes Linux AMD64 tests; declarations are C89-compatible, but other
platforms do not gain runtime acceptance from a successful source build alone.

Функция перемещает активную позицию открытого файла `r` либо `w`. Владение
дескриптором остаётся у вызывающего кода; синхронизация с чтением, записью и
закрытием — его обязанность. Параметры: файл, знаковое смещение в байтах,
начало отсчёта и обязательный указатель результата. Успех возвращает [`FVM_OK`](../include/flashvm/flashvm.h#L44)
и новую позицию; ошибка сохраняет логические позиции и значение результата.

Начало отсчёта: начало файла, текущая позиция либо конец. Отрицательное смещение
допустимо, если итоговая позиция неотрицательна. Переполнение проверяется до
сложения; на платформах с узким `long` большие смещения отклоняются. Режим `rw`
с независимыми чтением и дописыванием намеренно возвращает [`FVM_UNSUPPORTED`](../include/flashvm/flashvm.h#L48).
Это сохраняет прежнюю семантику файловых каналов FlashVM. Операция может
блокироваться в нативной файловой системе; это не реализация Styx или VFS.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_ERROR` | enum constant | [include/flashvm/flashvm.h:45](../include/flashvm/flashvm.h#L45) |
| `FVM_FILE_SEEK_CURRENT` | macro / define | [ports/file_port.h:52](../ports/file_port.h#L52) |
| `FVM_FILE_SEEK_END` | macro / define | [ports/file_port.h:53](../ports/file_port.h#L53) |
| `FVM_FILE_SEEK_START` | macro / define | [ports/file_port.h:51](../ports/file_port.h#L51) |
| `FVM_INVALID_ARGUMENT` | enum constant | [include/flashvm/flashvm.h:47](../include/flashvm/flashvm.h#L47) |
| `FVM_OK` | enum constant | [include/flashvm/flashvm.h:44](../include/flashvm/flashvm.h#L44) |
| `fvm_port_file_open` | function | [ports/file_port.c:44](../ports/file_port.c#L44) |
| `fvm_port_file_seek` | function | [ports/file_port.c:261](../ports/file_port.c#L261) |
| `FVM_PortFile` | struct | [ports/file_port.c:26](../ports/file_port.c#L26) |
| `FVM_UNSUPPORTED` | enum constant | [include/flashvm/flashvm.h:48](../include/flashvm/flashvm.h#L48) |
<!-- source-links:end -->
