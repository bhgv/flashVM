# fvm_styxrpc_filesystem_mount

[English](STYX_RPC_FILESYSTEM_MOUNT.md) · [Руководство](STYX_RPC_FILESYSTEM.ru.md)

```c
#include <flashvm/styxrpc_filesystem.h>
FVM_Result fvm_styxrpc_filesystem_mount(FVM_Context *context, const char *path,
    struct StyxClient *session);
```

Линкуйте `flashvm_styxrpc`. В потоке-владельце привяжите уже смонтированную `session` к пути вроде `/dis`. Контекст копирует путь и заимствует клиент до снятия монтирования либо освобождения. Каждый открытый fid удерживает клиент независимо. Хост может явно отключить транспорт. Формат пути, повторные пути и ошибки выделения обрабатываются общим API монтирования. Дополнительное подключение не открывается.
Передайте `/` в свежем контексте, чтобы обычные пути и файловые каналы обращались к подключённой файловой системе Dis. Корень действует до уничтожения контекста.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_Context` | struct | [src/core/runtime.hpp:257](../src/core/runtime.hpp#L257) |
| `FVM_Result` | enum | [include/flashvm/flashvm.h:42](../include/flashvm/flashvm.h#L42) |
| `fvm_styxrpc_filesystem_mount` | function | [include/flashvm/styxrpc_filesystem.h:22](../include/flashvm/styxrpc_filesystem.h#L22) |
<!-- source-links:end -->
