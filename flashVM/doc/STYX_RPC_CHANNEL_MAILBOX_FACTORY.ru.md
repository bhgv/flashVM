# fvm_styxrpc_channel_mailbox_factory

[English](STYX_RPC_CHANNEL_MAILBOX_FACTORY.md) · [Объявление](../include/flashvm/styxrpc_channel.h#L141) · [Реализация](../src/styxrpc/channel_mailbox.c#L284)

```c
#include <flashvm/styxrpc_channel.h>
FVM_Result fvm_styxrpc_channel_mailbox_factory(FVM_Context *context, const char *global, StyxRpcServer *reverse);
```

Линкуйте `flashvm_styxrpc`.

В потоке-владельце `context` зарегистрируйте `global` под непустым именем с завершающим NUL. Функция AS получает `(anchor, shape, slots)` и создаёт [встречную очередь](STYX_RPC_CHANNEL_MAILBOX.ru.md) на переданном сервере `reverse` того же двунаправленного монтирования. Храните сервер, пока функция доступна и выданные очереди действуют.

Возвращает [`FVM_OK`](../include/flashvm/flashvm.h#L44) при регистрации, [`FVM_INVALID_ARGUMENT`](../include/flashvm/flashvm.h#L47) при отсутствии обязательных аргументов либо ошибку регистрации нативной функции. Вызов из AS с неверными аргументами завершается ошибкой. Отказ публикации вызывает исключение AS.

[Руководство](STYX_RPC_CHANNEL_DELEGATION.ru.md) · [Нативная проверка](../tests/styxrpc/channel-mailbox.c)

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_Context` | struct | [src/core/runtime.hpp:257](../src/core/runtime.hpp#L257) |
| `FVM_INVALID_ARGUMENT` | enum constant | [include/flashvm/flashvm.h:47](../include/flashvm/flashvm.h#L47) |
| `FVM_OK` | enum constant | [include/flashvm/flashvm.h:44](../include/flashvm/flashvm.h#L44) |
| `FVM_Result` | enum | [include/flashvm/flashvm.h:42](../include/flashvm/flashvm.h#L42) |
| `fvm_styxrpc_channel_mailbox_factory` | function | [include/flashvm/styxrpc_channel.h:141](../include/flashvm/styxrpc_channel.h#L141) |
<!-- source-links:end -->
