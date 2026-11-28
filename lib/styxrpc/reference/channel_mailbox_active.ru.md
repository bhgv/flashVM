# styxrpc_channel_mailbox_active

[English](channel_mailbox_active.md) · [Объявление](../include/styxrpc/channel_mailbox.h#L45) · [Реализация](../src/channel_mailbox.c#L259)

```c
#include <styxrpc/channel_mailbox.h>
int styxrpc_channel_mailbox_active(StyxRpcChannelMailbox *mailbox);
```

Линкуйте `StyxRpc::serve`.

Возвращает 1, пока ресурс доступен, и 0 для NULL и после отзыва. Ресурс, ожидающий публикации, считается действующим. Заимствуйте `mailbox` на время проверки, удерживая свою ссылку владельца или ресурса. Использует мьютекс очереди без сетевого ввода-вывода. Параллельный clunk может отозвать ресурс после возврата. См. [создание результата](channel_mailbox_result_create.ru.md) и [проверку C](../tests/channel_mailbox.c).
