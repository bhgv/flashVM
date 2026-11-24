# fvm_filesystem_install

[English](FILESYSTEM_INSTALL.md) · [Руководство](STYX_RPC_FILESYSTEM.ru.md)

```c
#include <flashvm/filesystem.h>
FVM_Result fvm_filesystem_install(FVM_Context *context, const char *global);
```

Установите обычные операции FlashVM под свободным глобальным именем, например `fs`. Вызывайте в потоке-владельце. Имя заимствуется до возврата. Привязка сохраняется в контексте без файловых операций. Занятое имя и неверные аргументы дают [`FVM_INVALID_ARGUMENT`](../include/flashvm/flashvm.h#L47). Ошибки объектов и выделения памяти передаются вызывающему.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_Context` | struct | [src/core/runtime.hpp:257](../src/core/runtime.hpp#L257) |
| `fvm_filesystem_install` | function | [include/flashvm/filesystem.h:57](../include/flashvm/filesystem.h#L57) |
| `FVM_INVALID_ARGUMENT` | enum constant | [include/flashvm/flashvm.h:47](../include/flashvm/flashvm.h#L47) |
| `FVM_Result` | enum | [include/flashvm/flashvm.h:42](../include/flashvm/flashvm.h#L42) |
<!-- source-links:end -->
