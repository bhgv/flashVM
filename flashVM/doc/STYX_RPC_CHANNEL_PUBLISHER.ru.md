# fvm_styxrpc_channel_publisher

[English](STYX_RPC_CHANNEL_PUBLISHER.md) · [Объявление](../include/flashvm/styxrpc_channel.h#L47) · [Реализация](../src/styxrpc/channel_delegate.c#L120)

Подключите `<flashvm/styxrpc_channel.h>` и линкуйте `flashvm_styxrpc`.

```c
FVM_Result fvm_styxrpc_channel_publisher(FVM_Context *context, const char *global, StyxRpcServer *reverse);
```

Устанавливает глобальную нативную функцию в `context` под непустым именем `global`, завершённым NUL. Вызывайте в потоке-владельце FlashVM. В `reverse` передайте [сервер программ](../../lib/styxrpc/reference/program_server_create.ru.md) со встречной половины подключения Styx. Сервер заимствуется, пока функция доступна и действуют выданные ею публикации.

Возвращает [`FVM_OK`](../include/flashvm/flashvm.h#L44) при регистрации, [`FVM_INVALID_ARGUMENT`](../include/flashvm/flashvm.h#L47) при отсутствии обязательных параметров или статус регистратора при ошибке выделения либо привязки. Вызов из AS с `(anchor, queue)` выполняет [публикацию локальной очереди](STYX_RPC_CHANNEL_DELEGATE_LOCAL.ru.md). Успешный вызов возвращает непрозрачное значение PID. Недопустимая очередь или исчерпание слотов PID вызывают исключение AS. Партнёр получает возможность пользоваться каналом после отправки значения.

[Проверочный хост](../tests/styxrpc/channel-pid-graph.c) регистрирует `publishPort` перед передачей графа локальных очередей. [Руководство по делегированию](STYX_RPC_CHANNEL_DELEGATION.ru.md)

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
| `fvm_styxrpc_channel_publisher` | function | [include/flashvm/styxrpc_channel.h:47](../include/flashvm/styxrpc_channel.h#L47) |
<!-- source-links:end -->
