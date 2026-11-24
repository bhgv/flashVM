# fvm_styxrpc_object_image_create

[Русский](STYX_RPC_OBJECT_IMAGE_CREATE.ru.md) · [API](../include/flashvm/styxrpc_module.h) · [Source](../src/styxrpc/object_image.c) · [Guide](STYX_RPC_MODULES.md)

`flashvm_styxrpc`

```c
FVM_Result fvm_styxrpc_object_image_create(FVM_Context *context,
    const FVM_StyxRpcObjectSpec *spec, FVM_StyxRpcObjectImage **out);
```

## Contract

Give the owner context and a spec containing the named receiver, export descriptors, optional environment bindings, registry and controls. The image copies names and keeps the receiver GC-rooted. It borrows the registry and controls. Success writes an owned image. Failure preserves `out` and returns allocation, validation or GC status. All callbacks entering ActionScript run on the owner.

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
