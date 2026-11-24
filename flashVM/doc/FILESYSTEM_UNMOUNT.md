# fvm_filesystem_unmount

[Русский](FILESYSTEM_UNMOUNT.ru.md) · [Guide](STYX_RPC_FILESYSTEM.md)

```c
#include <flashvm/filesystem.h>
FVM_Result fvm_filesystem_unmount(FVM_Context *context, const char *path);
```

Remove the exact `path` binding on the context owner. Open descriptors keep their file identity and copied callbacks. Subsequent lookups use the remaining mounts and the native file backend. [`FVM_NOT_FOUND`](../include/flashvm/flashvm.h#L49) means the binding was absent. Invalid arguments return [`FVM_INVALID_ARGUMENT`](../include/flashvm/flashvm.h#L47). The operation requires no transport I/O.
An exclusive root mount stays active until context destruction. Attempting to unmount `/` returns [`FVM_UNSUPPORTED`](../include/flashvm/flashvm.h#L48).

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
