# fvm_styxrpc_module_loader_install

[English](STYX_RPC_MODULE_LOADER_INSTALL.md) · [API](../include/flashvm/styxrpc_module.h) · [Source](../src/styxrpc/module_import.c) · [Guide](STYX_RPC_MODULES.ru.md)

`flashvm_styxrpc`

```c
FVM_Result fvm_styxrpc_module_loader_install(FVM_Context *context, const char *global,
    const FVM_StyxRpcModuleCatalog *catalog);
```

## Контракт

Зарегистрируйте функцию сценария под выбранным глобальным именем. Передайте непустой неизменяемый каталог с различными относительными путями и корректными импортами. Функция принимает одну строку с длиной и необязательной начальной косой чертой. Неизвестный путь даёт [`FVM_NOT_FOUND`](../include/flashvm/flashvm.h#L49). Устанавливайте у владельца и сохраняйте каталог до уничтожения контекста.

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
