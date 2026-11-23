# fvm_filesystem_install

[Русский](FILESYSTEM_INSTALL.ru.md) · [Guide](STYX_RPC_FILESYSTEM.md)

```c
#include <flashvm/filesystem.h>
FVM_Result fvm_filesystem_install(FVM_Context *context, const char *global);
```

Install the ordinary FlashVM file operations under a free global name such as `fs`. Call on the context owner. The name is borrowed for installation. This copies the binding into the context and does no file I/O. An occupied name or invalid argument returns [`FVM_INVALID_ARGUMENT`](../include/flashvm/flashvm.h#L47). Allocation and object errors propagate. See the script operations in the filesystem guide.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_Context` | struct | [src/core/runtime.hpp:257](../src/core/runtime.hpp#L257) |
| `fvm_filesystem_install` | function | [include/flashvm/filesystem.h:55](../include/flashvm/filesystem.h#L55) |
| `FVM_INVALID_ARGUMENT` | enum constant | [include/flashvm/flashvm.h:47](../include/flashvm/flashvm.h#L47) |
| `FVM_Result` | enum | [include/flashvm/flashvm.h:42](../include/flashvm/flashvm.h#L42) |
<!-- source-links:end -->
