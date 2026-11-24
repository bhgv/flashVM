# fvm_filesystem_mount

[Русский](FILESYSTEM_MOUNT.ru.md) · [Guide](STYX_RPC_FILESYSTEM.md)

```c
#include <flashvm/filesystem.h>
FVM_Result fvm_filesystem_mount(FVM_Context *context, const char *path,
    const FVM_Filesystem *backend, void *state);
```

Install a provider called by ordinary path operations in the owner context. Supply one absolute component such as `/dis`, up to 255 bytes after the leading slash. The context copies the path and descriptors, and borrows `state` until unmount or destruction. The provider receives a relative path. An empty path addresses its root. Each successful open returns an independently owned descriptor. It must keep its own resources alive through close, which can run during GC. Calls are serialized by the context or by the file channel lock. Provide every callback. Invalid inputs and duplicate paths return [`FVM_INVALID_ARGUMENT`](../include/flashvm/flashvm.h#L47). Allocation failure returns [`FVM_OUT_OF_MEMORY`](../include/flashvm/flashvm.h#L46).
In a fresh context, `/` is an exclusive mount that resolves all paths, including relative paths. A second mount is rejected and context destruction releases the root.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_Context` | struct | [src/core/runtime.hpp:257](../src/core/runtime.hpp#L257) |
| `FVM_Filesystem` | struct | [include/flashvm/filesystem.h:31](../include/flashvm/filesystem.h#L31) |
| `fvm_filesystem_mount` | function | [include/flashvm/filesystem.h:46](../include/flashvm/filesystem.h#L46) |
| `FVM_INVALID_ARGUMENT` | enum constant | [include/flashvm/flashvm.h:47](../include/flashvm/flashvm.h#L47) |
| `FVM_OUT_OF_MEMORY` | enum constant | [include/flashvm/flashvm.h:46](../include/flashvm/flashvm.h#L46) |
| `FVM_Result` | enum | [include/flashvm/flashvm.h:42](../include/flashvm/flashvm.h#L42) |
<!-- source-links:end -->
