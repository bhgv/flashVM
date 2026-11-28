# styxrpc_channel_mailbox_result_create

[English](channel_mailbox_result_create.md) · [Объявление](../include/styxrpc/channel_mailbox.h#L39) · [Реализация](../src/channel_mailbox.c#L251)

```c
#include <styxrpc/channel_mailbox.h>
StyxRpcStatus styxrpc_channel_mailbox_result_create(const char *shape, unsigned slots,
    StyxRpcChannelMailbox **mailbox, StyxRpcChannelOutput *output);
```

Линкуйте `StyxRpc::serve`.

Создаёт результат модуля с двумя независимыми направлениями. `shape` — форма одного типа элемента в скобках с завершающим нулём. Допускаются копируемые значения и формы с каналами PID. `slots` от 0 до 8 для каждого направления, ноль задаёт rendezvous. Выданный ресурс читает ответы со смещения ноль и обрабатывает одну ветку на fid.

При `STYXRPC_OK` `output` владеет ресурсом, а `mailbox` получает отдельную ссылку. Передайте ресурс как [арендованный результат модуля](channel_handlers_create_with_results.ru.md). При clunk модуль отзывает все fid и вызывает освобождение ресурса. Владелец может [отправлять](channel_mailbox_send.ru.md) и [принимать](channel_mailbox_receive.ru.md) во время публикации. После отзыва оба вызова возвращают `STYXRPC_INVALID`. Освободите отдельную ссылку через [снятие ссылки владельца](channel_mailbox_owner_release.ru.md). Любая ссылка может освободиться первой. Неверные параметры или ошибка выделения дают `STYXRPC_INVALID` и сохраняют оба выхода. [Проверки C](../tests/channel_mailbox.c)

[Запускаемый пример C, Limbo и AS2](../examples/disvm/CHANNEL_RESULTS.ru.md).
