# fvm_styxrpc_export_int

[Русский](STYX_RPC_EXPORT_INT.ru.md) · [Declaration](../include/flashvm/styxrpc.h) · [Implementation](../src/styxrpc/scalar.c) · [Bridge setup](STYX_RPC.md)

Include `flashvm/styxrpc.h` and `styxrpc/scalar_module.h`, then link
`flashvm_styxrpc`.

```c
FVM_Result fvm_styxrpc_export_int(FVM_StyxRpcExport *binding, const char *name,
    struct StyxRpcExport *exported);
```

Set `binding` to the owner context and the target script function name. Pass
the wire export `name` and storage for `exported`. [`FVM_OK`](../include/flashvm/flashvm.h#L44) initializes a
`fn(int): int` descriptor. Bad arguments return [`FVM_INVALID_ARGUMENT`](../include/flashvm/flashvm.h#L47) and
preserve the descriptor. Publish it using the [scalar module API](https://github.com/bhgv/flashVM/blob/master/lib/styxrpc/reference/scalar_module_init.md).

Run the server on the FlashVM context's owner thread. The callback resolves
the named function on each call and converts its argument to an AS number. The
result must be an exact signed Int32. A script exception, absent function or
invalid result returns a framed remote error. The callback clears a script
exception produced by that call, leaving preexisting exceptions alone.
Keep the context, descriptor, binding and names alive through server
destruction. The server owns per-fid state and the delegated transport.

[Executable fixture](../tests/styxrpc_host.c) · [Import](STYX_RPC_IMPORT_INT.md)

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
| `fvm_styxrpc_export_int` | function | [include/flashvm/styxrpc.h:48](../include/flashvm/styxrpc.h#L48) |
| `FVM_StyxRpcExport` | struct | [include/flashvm/styxrpc.h:25](../include/flashvm/styxrpc.h#L25) |
<!-- source-links:end -->
