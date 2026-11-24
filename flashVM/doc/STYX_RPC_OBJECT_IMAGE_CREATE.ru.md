# fvm_styxrpc_object_image_create

[English](STYX_RPC_OBJECT_IMAGE_CREATE.md) · [API](../include/flashvm/styxrpc_module.h) · [Source](../src/styxrpc/object_image.c) · [Guide](STYX_RPC_MODULES.ru.md)

`flashvm_styxrpc`

```c
FVM_Result fvm_styxrpc_object_image_create(FVM_Context *context,
    const FVM_StyxRpcObjectSpec *spec, FVM_StyxRpcObjectImage **out);
```

## Контракт

Передайте контекст владельца и описатель с именем, объектом, экспортами, необязательными переменными, реестром и управлением. Образ копирует имена и удерживает объект корнем GC. Реестр и управление заимствуются. Успех даёт новый образ, ошибка проверки, выделения или GC сохраняет `out`. Колбэки ActionScript исполняются у владельца.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_Context` | struct | [src/core/runtime.hpp:257](../src/core/runtime.hpp#L257) |
| `FVM_Result` | enum | [include/flashvm/flashvm.h:42](../include/flashvm/flashvm.h#L42) |
| `fvm_styxrpc_object_image_create` | function | [include/flashvm/styxrpc_module.h:49](../include/flashvm/styxrpc_module.h#L49) |
| `FVM_StyxRpcObjectImage` | struct | [src/styxrpc/object_image.c:31](../src/styxrpc/object_image.c#L31) |
| `FVM_StyxRpcObjectSpec` | struct | [include/flashvm/styxrpc_module.h:34](../include/flashvm/styxrpc_module.h#L34) |
<!-- source-links:end -->
