# fvm_styxrpc_object_image_destroy

[English](STYX_RPC_OBJECT_IMAGE_DESTROY.md) · [API](../include/flashvm/styxrpc_module.h) · [Source](../src/styxrpc/object_image.c) · [Guide](STYX_RPC_MODULES.ru.md)

`flashvm_styxrpc`

```c
void fvm_styxrpc_object_image_destroy(FVM_StyxRpcObjectImage *image);
```

## Контракт

После закрытия всех серверов освободите образ у владельца. Вызов снимает удержание объекта и освобождает копии метаданных до уничтожения контекста. Допускается NULL.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_styxrpc_object_image_destroy` | function | [include/flashvm/styxrpc_module.h:60](../include/flashvm/styxrpc_module.h#L60) |
| `FVM_StyxRpcObjectImage` | struct | [src/styxrpc/object_image.c:31](../src/styxrpc/object_image.c#L31) |
<!-- source-links:end -->
