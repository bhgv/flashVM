# fvm_styxrpc_export_managed

[Русский](STYX_RPC_EXPORT_MANAGED.ru.md) · [Header](../include/flashvm/styxrpc_managed.h) · [Implementation](../src/styxrpc/managed.c) · [Bridge](STYX_RPC.md)

Include `flashvm/styxrpc_managed.h` and link `flashvm_styxrpc`.

```c
FVM_Result fvm_styxrpc_export_managed(FVM_StyxRpcManagedExport *binding, const char *name,
    uint32_t signature, StyxRpcManagedExport *exported);
```

Set `binding` to the owner context, AS implementation name, compiled schema
and application-owned encoded result and update staging of 1–65536 bytes. Pass the
wire function `name`, compiled `signature` and caller-owned output.
[`FVM_OK`](../include/flashvm/flashvm.h#L44) initializes a validated synchronous descriptor. Invalid
inputs return [`FVM_INVALID_ARGUMENT`](../include/flashvm/flashvm.h#L47) and preserve `exported`. Publish
the descriptor with
[managed module publication](https://github.com/bhgv/flashVM/blob/master/lib/styxrpc/reference/managed_module_init.md).

Dispatch on the FlashVM context's owner thread. The callback roots newly
decoded arguments and the result during conversion and calls the named
AS function. An invalid result or script exception produces a remote
error. Existing pending exceptions prevent admission. Arguments and
results are [mapped by schema](STYX_RPC_IMPORT_MANAGED.md), including
[ordinary ref copy-back](STYX_RPC_REFS.md).
The staging buffer is borrowed through the synchronous encoding of the
reply. Keep it separate from all transport buffers. Keep the descriptor
and binding data alive and stable until server destruction.

A result in a `big` position accepts a safe integer Number or a signed decimal String, with the same rules for fields of arrays and tuples. The Limbo caller receives a signed 64-bit integer. [Value type reference](STYX_RPC_VALUE_TYPES.md)

[Executable fixture](../tests/styxrpc/managed.c) · [Import](STYX_RPC_IMPORT_MANAGED.md)

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
| `fvm_styxrpc_export_managed` | function | [include/flashvm/styxrpc_managed.h:32](../include/flashvm/styxrpc_managed.h#L32) |
| `FVM_StyxRpcManagedExport` | struct | [include/flashvm/styxrpc_managed.h:14](../include/flashvm/styxrpc_managed.h#L14) |
<!-- source-links:end -->
