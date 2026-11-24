# fvm_styxrpc_object_image_module

[Русский](STYX_RPC_OBJECT_IMAGE_MODULE.ru.md) · [API](../include/flashvm/styxrpc_module.h) · [Source](../src/styxrpc/object_image.c) · [Guide](STYX_RPC_MODULES.md)

`flashvm_styxrpc`

```c
StyxRpcModule *fvm_styxrpc_object_image_module(FVM_StyxRpcObjectImage *image);
```

## Contract

Return the borrowed SDK backing for `styxrpc_module_tree`, or NULL for a NULL image. Use it only until image destruction. Close the server before destroying the image.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_StyxRpcObjectImage` | struct | [src/styxrpc/object_image.c:31](../src/styxrpc/object_image.c#L31) |
<!-- source-links:end -->
