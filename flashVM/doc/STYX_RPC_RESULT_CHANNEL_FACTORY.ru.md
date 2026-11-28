# fvm_styxrpc_result_channel_factory

[English](STYX_RPC_RESULT_CHANNEL_FACTORY.md) · [Руководство](STYX_RPC_RESULT_CHANNELS.ru.md) · [Реализация](../src/styxrpc/channel_result_endpoint.c#L293) · [Заголовок](../include/flashvm/styxrpc_channel.h)

## Синтаксис

```c
#include <flashvm/styxrpc_channel.h>
FVM_Result fvm_styxrpc_result_channel_factory(FVM_Context *context, const char *global);
```

Подключите хостовую цель `flashvm_styxrpc`. Вызовите у владельца FlashVM, передав действительный `context` и непустое имя функции `global` с завершающим нулём. Оба параметра заимствуются для регистрации. При [`FVM_OK`](../include/flashvm/flashvm.h#L44) функция AS получает [контракт `vfs.resultChannel`](STYX_RPC_RESULT_CHANNEL.ru.md). Неверный контекст или имя возвращают [`FVM_INVALID_ARGUMENT`](../include/flashvm/flashvm.h#L47). Возможна ошибка регистрации VM или выделения памяти. Сетевого обмена при регистрации нет.

```c
FVM_Result status = fvm_styxrpc_result_channel_factory(context, "newResultChannel");
```

Используйте `newResultChannel("i", 0)` в подключённом фильме. Верните его из [экспортированной функции](STYX_RPC_CHANNEL_EXPORT.ru.md) с результатом `(c[i])`. Удерживайте привязку колбэка и провайдер на протяжении экспорта.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_Context` | struct | [src/core/runtime.hpp:260](../src/core/runtime.hpp#L260) |
| `FVM_INVALID_ARGUMENT` | enum constant | [include/flashvm/flashvm.h:47](../include/flashvm/flashvm.h#L47) |
| `FVM_OK` | enum constant | [include/flashvm/flashvm.h:44](../include/flashvm/flashvm.h#L44) |
| `FVM_Result` | enum | [include/flashvm/flashvm.h:42](../include/flashvm/flashvm.h#L42) |
| `fvm_styxrpc_result_channel_factory` | function | [include/flashvm/styxrpc_channel.h:166](../include/flashvm/styxrpc_channel.h#L166) |
<!-- source-links:end -->
