# styxrpc_named_peer_client

[English](named_peer_client.md) · [Заголовок](../include/styxrpc/named_peer.h#L58) · [Хост имён](named_peer_host.ru.md)

```c
#include <styxrpc/named_peer.h>

struct StyxClient *styxrpc_named_peer_client(StyxRpcNamedPeer *peer);
```

Заимствовать локальное подключение Styx из живого `peer`, возвращённого
[named_peer_host](named_peer_host.ru.md). При NULL возвращается NULL.
Вызов не выполняет I/O. Результат используется для
[именованных каналов](named_channel.ru.md) и [общих ячеек](named_cell.ru.md).
Открытые fid нужно закрыть. Владельцем подключения остаётся `peer`.
Закончите операции с ним до уничтожения владельца. Состояние соединения
подтверждается результатом каждой операции. Функция входит в хостовую
цель `StyxRpc::serve`.
