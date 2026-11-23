# fvm_styxrpc_export_graph

[Русский](STYX_RPC_EXPORT_GRAPH.ru.md) · [Header](../include/flashvm/styxrpc_graph.h) · [Implementation](../src/styxrpc/graph.c) · [Graph mapping](STYX_RPC_GRAPH.md)

Include `flashvm/styxrpc_graph.h` and link `flashvm_styxrpc`.

```c
FVM_Result fvm_styxrpc_export_graph(FVM_StyxRpcManagedExport *binding, const char *name,
    uint32_t signature, StyxRpcGraphExport *exported);
```

Build an export descriptor for the compiled `name` and `signature`. Provide a FlashVM owner, AS function name, complete schema and writable result buffer (up to 64 KiB) in `binding`. Publish `exported` through the [libstyxrpc graph module](../../lib/styxrpc/reference/graph_module_init.md). Drive the server on the FlashVM context owner. Keep the binding, name and storage alive until after server teardown. Calls must be serialized on the owner while using the same buffer.

[`FVM_OK`](../include/flashvm/flashvm.h#L44) reports a prepared descriptor. [`FVM_INVALID_ARGUMENT`](../include/flashvm/flashvm.h#L47) reports invalid binding data. An AS exception or a result that fails conversion produces a remote error response.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_INVALID_ARGUMENT` | enum constant | [include/flashvm/flashvm.h:47](../include/flashvm/flashvm.h#L47) |
| `FVM_OK` | enum constant | [include/flashvm/flashvm.h:44](../include/flashvm/flashvm.h#L44) |
| `FVM_Result` | enum | [include/flashvm/flashvm.h:42](../include/flashvm/flashvm.h#L42) |
| `fvm_styxrpc_export_graph` | function | [include/flashvm/styxrpc_graph.h:22](../include/flashvm/styxrpc_graph.h#L22) |
| `FVM_StyxRpcManagedExport` | struct | [include/flashvm/styxrpc_managed.h:14](../include/flashvm/styxrpc_managed.h#L14) |
<!-- source-links:end -->
