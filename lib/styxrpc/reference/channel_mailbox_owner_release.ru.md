# styxrpc_channel_mailbox_owner_release

[English](channel_mailbox_owner_release.md) · [Объявление](../include/styxrpc/channel_mailbox.h#L50) · [Реализация](../src/channel_mailbox.c#L271)

```c
#include <styxrpc/channel_mailbox.h>
void styxrpc_channel_mailbox_owner_release(StyxRpcChannelMailbox *mailbox);
```

Линкуйте `StyxRpc::serve`.

Освобождает отдельную ссылку владельца, выданную при [создании результата](channel_mailbox_result_create.ru.md). До вызова прекратите пользоваться `mailbox` через эту ссылку. NULL и заимствованный дескриптор из конструктора встречного PID допустимы. Вызов обходится без сетевого ввода-вывода. Если модуль удерживает свою ссылку ресурса, он может обслуживать fid до clunk. Последняя ссылка освобождает память. Вызывайте один раз на отдельную ссылку владельца. [Проверка C](../tests/channel_mailbox.c)
