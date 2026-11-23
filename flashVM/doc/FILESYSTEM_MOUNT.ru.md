# fvm_filesystem_mount

[English](FILESYSTEM_MOUNT.md) · [Руководство](STYX_RPC_FILESYSTEM.ru.md)

```c
#include <flashvm/filesystem.h>
FVM_Result fvm_filesystem_mount(FVM_Context *context, const char *path,
    const FVM_Filesystem *backend, void *state);
```

Установите провайдер обычных операций с путями в контексте владельца. Передайте один абсолютный компонент вроде `/dis` до 255 байтов после начальной косой черты. Контекст копирует путь и описатели и заимствует `state` до снятия монтирования или освобождения. Провайдер получает относительный путь. Пустой путь адресует корень. Успешное открытие передаёт независимый дескриптор, который удерживает свои ресурсы до закрытия, в том числе при GC. Вызовы сериализуются владельцем контекста либо мьютексом файлового канала. Передайте все колбэки. Неверный ввод и повторный путь дают [`FVM_INVALID_ARGUMENT`](../include/flashvm/flashvm.h#L47), нехватка памяти — [`FVM_OUT_OF_MEMORY`](../include/flashvm/flashvm.h#L46).

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_Context` | struct | [src/core/runtime.hpp:257](../src/core/runtime.hpp#L257) |
| `FVM_Filesystem` | struct | [include/flashvm/filesystem.h:31](../include/flashvm/filesystem.h#L31) |
| `fvm_filesystem_mount` | function | [include/flashvm/filesystem.h:44](../include/flashvm/filesystem.h#L44) |
| `FVM_INVALID_ARGUMENT` | enum constant | [include/flashvm/flashvm.h:47](../include/flashvm/flashvm.h#L47) |
| `FVM_OUT_OF_MEMORY` | enum constant | [include/flashvm/flashvm.h:46](../include/flashvm/flashvm.h#L46) |
| `FVM_Result` | enum | [include/flashvm/flashvm.h:42](../include/flashvm/flashvm.h#L42) |
<!-- source-links:end -->
