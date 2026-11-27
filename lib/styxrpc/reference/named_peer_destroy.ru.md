# styxrpc_named_peer_destroy

[English](named_peer_destroy.md) · [Заголовок](../include/styxrpc/named_peer.h#L65) · [Хост имён](named_peer_host.ru.md)

```c
#include <styxrpc/named_peer.h>

void styxrpc_named_peer_destroy(StyxRpcNamedPeer *peer);
```

Освободить принадлежащий вызывающему `peer` от
[named_peer_host](named_peer_host.ru.md). NULL допустим. Хост останавливает
оба диспетчера, прерывает ожидания локального клиента, дожидается потоков
и освобождает локальное подключение. Вызов может ждать завершения серверов.
Вызывайте в потоке хоста после окончания операций заимствованного клиента.
Сохраняйте реестр и состояние его колбэков до возврата. Функция входит в
хостовую цель `StyxRpc::serve`.
