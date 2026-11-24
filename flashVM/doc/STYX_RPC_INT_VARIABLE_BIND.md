# fvm_styxrpc_int_variable_bind

[Русский](STYX_RPC_INT_VARIABLE_BIND.ru.md) · [Guide](STYX_RPC_FILESYSTEM.md)

```c
#include <flashvm/styxrpc_filesystem.h>
FVM_Result fvm_styxrpc_int_variable_bind(FVM_Context *context, FVM_Object *object, const char *name,
    struct StyxClient *session, const char *path, int writable);
```

Link `flashvm_styxrpc`. On the context owner, install a new property `name` on `object` for the native signed 32-bit variable selected by the relative `path` on attached `session`. The file must advertise four bytes. Pass `writable=0` for a read-only property or 1 for read and write. The exact fid is retained until the property ceases to hold its GC root. Each ordinary get reads fresh data. An assignment validates an exact int and sends one whole update. Failed or uncertain transport accesses raise exceptions. An uncertain write has no automatic replay. Invalid arguments or a duplicate property return [`FVM_INVALID_ARGUMENT`](../include/flashvm/flashvm.h#L47). Failure to open the exact file returns [`FVM_ERROR`](../include/flashvm/flashvm.h#L45).

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_Context` | struct | [src/core/runtime.hpp:257](../src/core/runtime.hpp#L257) |
| `FVM_ERROR` | enum constant | [include/flashvm/flashvm.h:45](../include/flashvm/flashvm.h#L45) |
| `FVM_INVALID_ARGUMENT` | enum constant | [include/flashvm/flashvm.h:47](../include/flashvm/flashvm.h#L47) |
| `FVM_Object` | struct | [src/core/runtime.hpp:133](../src/core/runtime.hpp#L133) |
| `FVM_Result` | enum | [include/flashvm/flashvm.h:42](../include/flashvm/flashvm.h#L42) |
| `fvm_styxrpc_int_variable_bind` | function | [include/flashvm/styxrpc_filesystem.h:29](../include/flashvm/styxrpc_filesystem.h#L29) |
<!-- source-links:end -->
