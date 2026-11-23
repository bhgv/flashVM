# fvm_styxrpc_filesystem_mount

[Русский](STYX_RPC_FILESYSTEM_MOUNT.ru.md) · [Guide](STYX_RPC_FILESYSTEM.md)

```c
#include <flashvm/styxrpc_filesystem.h>
FVM_Result fvm_styxrpc_filesystem_mount(FVM_Context *context, const char *path,
    struct StyxClient *session);
```

Link `flashvm_styxrpc`. On the context owner, bind the already attached `session` to a path such as `/dis`. The context copies the path and borrows the client until unmount or destruction. Each opened fid independently retains the attached client. The host can shut down the transport explicitly. Use the generic mount rules for path spelling, duplicate paths and allocation errors. No additional session is opened.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_Context` | struct | [src/core/runtime.hpp:257](../src/core/runtime.hpp#L257) |
| `FVM_Result` | enum | [include/flashvm/flashvm.h:42](../include/flashvm/flashvm.h#L42) |
| `fvm_styxrpc_filesystem_mount` | function | [include/flashvm/styxrpc_filesystem.h:20](../include/flashvm/styxrpc_filesystem.h#L20) |
<!-- source-links:end -->
