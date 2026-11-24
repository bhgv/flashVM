# fvm_filesystem_unmount

[English](FILESYSTEM_UNMOUNT.md) · [Руководство](STYX_RPC_FILESYSTEM.ru.md)

```c
#include <flashvm/filesystem.h>
FVM_Result fvm_filesystem_unmount(FVM_Context *context, const char *path);
```

Снимите точную привязку `path` в потоке-владельце контекста. Открытые дескрипторы сохраняют свои файлы и копии колбэков. Следующие обращения используют оставшиеся точки монтирования и нативную ФС. Если привязки нет, возвращается [`FVM_NOT_FOUND`](../include/flashvm/flashvm.h#L49). Неверный ввод даёт [`FVM_INVALID_ARGUMENT`](../include/flashvm/flashvm.h#L47). Транспортные операции не требуются.
Исключительный корень действует до уничтожения контекста. Попытка снять `/` возвращает [`FVM_UNSUPPORTED`](../include/flashvm/flashvm.h#L48).

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_Context` | struct | [src/core/runtime.hpp:257](../src/core/runtime.hpp#L257) |
| `fvm_filesystem_unmount` | function | [include/flashvm/filesystem.h:52](../include/flashvm/filesystem.h#L52) |
| `FVM_INVALID_ARGUMENT` | enum constant | [include/flashvm/flashvm.h:47](../include/flashvm/flashvm.h#L47) |
| `FVM_NOT_FOUND` | enum constant | [include/flashvm/flashvm.h:49](../include/flashvm/flashvm.h#L49) |
| `FVM_Result` | enum | [include/flashvm/flashvm.h:42](../include/flashvm/flashvm.h#L42) |
| `FVM_UNSUPPORTED` | enum constant | [include/flashvm/flashvm.h:48](../include/flashvm/flashvm.h#L48) |
<!-- source-links:end -->
