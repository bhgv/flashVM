# fvm_styxrpc_module_loader_install

[Русский](STYX_RPC_MODULE_LOADER_INSTALL.ru.md) · [API](../include/flashvm/styxrpc_module.h) · [Source](../src/styxrpc/module_import.c) · [Guide](STYX_RPC_MODULES.md)

`flashvm_styxrpc`

```c
FVM_Result fvm_styxrpc_module_loader_install(FVM_Context *context, const char *global,
    const FVM_StyxRpcModuleCatalog *catalog);
```

## Contract

Register a script function at the selected global name. Supply a nonempty immutable catalog with distinct relative paths and valid import specifications. The script takes a single counted string path and accepts one optional leading slash. An unknown path returns [`FVM_NOT_FOUND`](../include/flashvm/flashvm.h#L49). Install on the owner and keep the catalog and its entries alive until context teardown.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_Context` | struct | [src/core/runtime.hpp:257](../src/core/runtime.hpp#L257) |
| `FVM_NOT_FOUND` | enum constant | [include/flashvm/flashvm.h:49](../include/flashvm/flashvm.h#L49) |
| `FVM_Result` | enum | [include/flashvm/flashvm.h:42](../include/flashvm/flashvm.h#L42) |
| `fvm_styxrpc_module_loader_install` | function | [include/flashvm/styxrpc_module.h:87](../include/flashvm/styxrpc_module.h#L87) |
| `FVM_StyxRpcModuleCatalog` | struct | [include/flashvm/styxrpc_module.h:78](../include/flashvm/styxrpc_module.h#L78) |
<!-- source-links:end -->
