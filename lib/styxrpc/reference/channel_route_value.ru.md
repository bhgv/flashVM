# styxrpc_channel_route_value

[English](channel_route_value.md) · [libstyxrpc](../README.ru.md)

## Синтаксис

```c
#include <styxrpc/channel_route.h>
StyxRpcStatus styxrpc_channel_route_value(const StyxRpcChannelRoute *route, void *bytes,
    size_t capacity, size_t *used);
```

Запишите лист вложенного канала. `route` содержит объявленную схему
элемента, ключ и признак ёмкости. Для PID задайте PID со старшим битом,
для MODULE ненулевой 16-байтовый токен, для CURRENT оба поля нулевые.
Неиспользуемые поля адреса должны быть нулевыми. `bytes` указывает на
буфер приложения ёмкостью `capacity` байтов.

При `STYXRPC_OK` в `*used` записывается длина полного листа. При
`STYXRPC_INVALID` описатель или буфер неверен, в переданном `*used` ноль.
Включите этот лист в типизированный элемент. Сетевой тег 33 означает PID,
35 означает MODULE, 24/34 означают обычный или синхронный ключ CURRENT.
Приложение публикует ресурс отдельно и удерживает его на время ожидающих
операций. Подключите `StyxRpc::rpc`.

[styxrpc_channel_read_routed](channel_read_routed.ru.md) · [Пример](../examples/disvm/routed_client.c) · [Исходник](../src/channel_route.c)
