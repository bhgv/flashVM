# styxrpc_channel_mailbox_create

[English](channel_mailbox_create.md) · [Объявление](../include/styxrpc/channel_mailbox.h#L20) · [Реализация](../src/channel_mailbox.c#L185)

```c
#include <styxrpc/channel_mailbox.h>
StyxRpcStatus styxrpc_channel_mailbox_create(const char *shape, unsigned slots,
    StyxRpcChannelMailbox **mailbox, StyxRpcChannelOutput *output);
```

Линкуйте `StyxRpc::serve`.

Создаёт встречный ресурс UniBus с двумя независимыми FIFO по `slots` мест. Допускается от 1 до 8 мест. `shape` задаёт схему одного типизированного элемента с полями-каналами в скобках, например `(c[i])`, с завершающим NUL. Поля-каналы содержат полномочия модулей или nil. Схема копируется. Управляемые ссылки и гибридные дескрипторы находятся за пределами профиля.

При `STYXRPC_OK` ресурс принадлежит `output`, а `mailbox` заимствует его. При неверных параметрах или ошибке выделения возвращается `STYXRPC_INVALID`, оба результата сохраняются. Зарегистрируйте `output.resource` на [сервере программ](program_server_grant.ru.md). Используйте выданный PID и `output.key` с признаком буферизованного канала. Каждый открытый fid принимает одну нативную ветку. Ответ читается со смещения, равного длине запроса.

Владелец может [отправлять](channel_mailbox_send.ru.md) и [принимать](channel_mailbox_receive.ru.md) при работающем диспетчере. Колбэки сервера перемещают копии кадров. Хост может [проверить токены](channel_lease_import_message.ru.md) под соответствующей арендой результата при извлечении кадра. Отзовите PID и завершите вызовы владельца, затем вызовите `output.release(output.resource.context)`.

[Проверка на C](../tests/channel_mailbox.c)
