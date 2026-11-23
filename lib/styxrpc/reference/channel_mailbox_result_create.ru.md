# styxrpc_channel_mailbox_result_create

[English](channel_mailbox_result_create.md) · [Объявление](../include/styxrpc/channel_mailbox.h#L39) · [Реализация](../src/channel_mailbox.c#L249)

```c
#include <styxrpc/channel_mailbox.h>
StyxRpcStatus styxrpc_channel_mailbox_result_create(const char *shape, unsigned slots,
    StyxRpcChannelMailbox **mailbox, StyxRpcChannelOutput *output);
```

Линкуйте `StyxRpc::serve`.

Создаёт типизированный результат модуля с двумя отдельными FIFO. `shape` задаёт в скобках схему одного значения с каналами и завершающим NUL. `slots` задаёт от 1 до 8 мест на направление. Каждое ненулевое поле-канал содержит отдельный PID. Проверенный ресурс модуля выдаёт ответы со смещения ноль и принимает одну ветку на fid.

При `STYXRPC_OK` `output` владеет ресурсом, а `mailbox` получает отдельную ссылку. Передайте ресурс как [арендованный результат модуля](channel_handlers_create_with_results.ru.md). При clunk модуль отзывает все fid и вызывает освобождение ресурса. Владелец может [отправлять](channel_mailbox_send.ru.md) и [принимать](channel_mailbox_receive.ru.md) во время публикации. После отзыва оба вызова возвращают `STYXRPC_INVALID`. Освободите отдельную ссылку через [снятие ссылки владельца](channel_mailbox_owner_release.ru.md). Любая ссылка может освободиться первой. Неверные параметры или ошибка выделения дают `STYXRPC_INVALID` и сохраняют оба выхода. [Проверки C](../tests/channel_mailbox.c)
