# styxrpc_channel_mailbox_receive

[English](channel_mailbox_receive.md) · [Объявление](../include/styxrpc/channel_mailbox.h#L31) · [Реализация](../src/channel_mailbox.c#L271)

```c
#include <styxrpc/channel_mailbox.h>
StyxRpcStatus styxrpc_channel_mailbox_receive(StyxRpcChannelMailbox *mailbox,
    void *bytes, size_t capacity, size_t *size);
```

Линкуйте `StyxRpc::serve`.

Копирует и извлекает первый кадр, отправленный партнёром в открытую `mailbox`. Передайте доступный для записи буфер `bytes` ёмкостью `capacity` байт и обязательный выход длины `size`. При `STYXRPC_OK` записывается полная длина кадра и освобождается слот. `STYXRPC_MORE` означает пустой FIFO. При неверных параметрах или малом буфере `STYXRPC_INVALID` сохраняет очередь и выход длины.

Владелец и диспетчер используют общий мьютекс очереди. Вызов перемещает байты и может завершить ожидающую запись партнёра. Он обходится без ожидания входящего сообщения и сетевого ввода-вывода. Чтобы использовать каналы модулей из входящего кадра, [импортируйте сообщение](channel_lease_import_message.ru.md) в потоке-владельце под исходной арендой.

[Проверка на C](../tests/channel_mailbox.c)
