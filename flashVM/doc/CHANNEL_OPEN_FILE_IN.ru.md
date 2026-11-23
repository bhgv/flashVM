# fvm_channel_open_file_in

[English](CHANNEL_OPEN_FILE_IN.md) · [Руководство](STYX_RPC_FILESYSTEM.ru.md)

```c
#include <flashvm/channel.h>
FVM_Result fvm_channel_open_file_in(FVM_Context *context, const char *path, const char *mode, FVM_Channel **out);
```

Откройте обычный файловый канал с тем же форматом, что у [`fvm_channel_open_file`](../include/flashvm/channel.h#L63), используя контекст владельца для разрешения пути. Нулевой контекст разрешает только нативные пути. Путь и режим заимствуются до возврата. Успех передаёт собственный канал в `out`. Ошибка обнуляет переданный выход. Режимы зависят от выбранной ФС. Открытие может блокироваться. Освобождайте канал через [`fvm_channel_release`](../include/flashvm/channel.h#L96).

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_Channel` | struct | [src/capi/channel_c.c:30](../src/capi/channel_c.c#L30) |
| `fvm_channel_open_file` | function | [include/flashvm/channel.h:63](../include/flashvm/channel.h#L63) |
| `fvm_channel_open_file_in` | function | [include/flashvm/channel.h:154](../include/flashvm/channel.h#L154) |
| `fvm_channel_release` | function | [include/flashvm/channel.h:96](../include/flashvm/channel.h#L96) |
| `FVM_Context` | struct | [src/core/runtime.hpp:257](../src/core/runtime.hpp#L257) |
| `FVM_Result` | enum | [include/flashvm/flashvm.h:42](../include/flashvm/flashvm.h#L42) |
<!-- source-links:end -->
