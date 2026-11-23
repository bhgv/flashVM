# styxrpc_channel_mailbox_send

[English](channel_mailbox_send.md) · [Объявление](../include/styxrpc/channel_mailbox.h#L26) · [Реализация](../src/channel_mailbox.c#L298)

```c
#include <styxrpc/channel_mailbox.h>
StyxRpcStatus styxrpc_channel_mailbox_send(StyxRpcChannelMailbox *mailbox, const void *bytes, size_t size);
```

Линкуйте `StyxRpc::serve`.

Копирует `size` байт из `bytes` в исходящий FIFO открытой `mailbox`. Буфер содержит один полный элемент по схеме очереди с полномочиями модулей или nil в полях-каналах. Элемент должен помещаться в штатный ответ. Функция проверяет кадр и заимствует буфер на время вызова. Владелец передаёт полномочия, предназначенные для этого соединения.

Возвращает `STYXRPC_OK` после постановки в очередь или исполнения ожидающего чтения партнёра, `STYXRPC_MORE` при заполнении FIFO и `STYXRPC_INVALID` для неверного или неподдерживаемого входа. Вызов использует мьютекс очереди и обходится без сетевого ввода-вывода. Партнёр может прочитать готовый ответ на своём fid. Удерживайте очередь до возврата.

[Проверка на C](../tests/channel_mailbox.c)
