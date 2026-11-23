# fvm_styxrpc_import_graph

[Русский](STYX_RPC_IMPORT_GRAPH.ru.md) · [Header](../include/flashvm/styxrpc_graph.h) · [Implementation](../src/styxrpc/graph.c) · [Graph mapping](STYX_RPC_GRAPH.md)

Include `flashvm/styxrpc_graph.h` and link `flashvm_styxrpc`.

```c
FVM_Result fvm_styxrpc_import_graph(FVM_Context *context, const char *global,
    StyxRpcManagedTarget *target);
```

Register `global` as a synchronous callable on the dedicated FlashVM owner `context`. Supply a mounted session, relative module path, export name, signature, both schema lists and bounded reply wait in `target`. For a scoped export set the delegated PID and keep its reverse context published through the call.

[`FVM_OK`](../include/flashvm/flashvm.h#L44) reports local registration. Invalid configuration returns [`FVM_INVALID_ARGUMENT`](../include/flashvm/flashvm.h#L47). A call can fail with a script exception on argument, remote, transport or local copy-back errors. A local failure can occur after execution at the far end. Keep the owner, target and its text fields alive while callable. The owner may clear the session between calls before closing it.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_Context` | struct | [src/core/runtime.hpp:257](../src/core/runtime.hpp#L257) |
| `FVM_INVALID_ARGUMENT` | enum constant | [include/flashvm/flashvm.h:47](../include/flashvm/flashvm.h#L47) |
| `FVM_OK` | enum constant | [include/flashvm/flashvm.h:44](../include/flashvm/flashvm.h#L44) |
| `FVM_Result` | enum | [include/flashvm/flashvm.h:42](../include/flashvm/flashvm.h#L42) |
| `fvm_styxrpc_import_graph` | function | [include/flashvm/styxrpc_graph.h:16](../include/flashvm/styxrpc_graph.h#L16) |
<!-- source-links:end -->
