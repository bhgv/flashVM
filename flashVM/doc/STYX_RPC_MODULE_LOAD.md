# fvm_styxrpc_module_load

[Русский](STYX_RPC_MODULE_LOAD.ru.md) · [API](../include/flashvm/styxrpc_module.h) · [Source](../src/styxrpc/module_import.c) · [Guide](STYX_RPC_MODULES.md)

`flashvm_styxrpc`

```c
FVM_Result fvm_styxrpc_module_load(FVM_Context *context,
    const FVM_StyxRpcLoadSpec *spec, FVM_Value *out);
```

## Contract

Supply the owner context, attached session, relative module path, compiled imports, scoped PID when required, call wait budget in milliseconds and optional channel argument bus. The call blocks for interface verification and copies the accepted contract to an ordinary AS object. It returns [`FVM_OK`](../include/flashvm/flashvm.h#L44) with the object in `out`, [`FVM_NOT_FOUND`](../include/flashvm/flashvm.h#L49) for a failed remote verification, or a local validation or allocation status. Root the object normally. Keep the session and argument bus alive during use.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_Context` | struct | [src/core/runtime.hpp:257](../src/core/runtime.hpp#L257) |
| `FVM_NOT_FOUND` | enum constant | [include/flashvm/flashvm.h:49](../include/flashvm/flashvm.h#L49) |
| `FVM_OK` | enum constant | [include/flashvm/flashvm.h:44](../include/flashvm/flashvm.h#L44) |
| `FVM_Result` | enum | [include/flashvm/flashvm.h:42](../include/flashvm/flashvm.h#L42) |
| `fvm_styxrpc_module_load` | function | [include/flashvm/styxrpc_module.h:76](../include/flashvm/styxrpc_module.h#L76) |
| `FVM_StyxRpcLoadSpec` | struct | [include/flashvm/styxrpc_module.h:62](../include/flashvm/styxrpc_module.h#L62) |
| `FVM_Value` | struct | [include/flashvm/value.h:26](../include/flashvm/value.h#L26) |
<!-- source-links:end -->
