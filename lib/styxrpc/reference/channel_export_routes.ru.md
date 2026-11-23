# styxrpc_channel_export_routes

[English](channel_export_routes.md) · [libstyxrpc](../README.ru.md)

## Синтаксис

```c
#include <styxrpc/channel_route.h>
StyxRpcStatus styxrpc_channel_export_routes(const char *shape, const void *bytes, size_t size,
    StyxRpcChannelRoutes *routes);
```

Соберите адреса каналов модуля из подтверждённого результата функции.
Передайте объявленную схему результата `shape`, полное закодированное поле
`bytes` с тегом 90 из пары результата и изменений и точный размер `size`
в байтах. В схему входят внешние скобки списка значений. Кортеж из канала и
целого имеет схему `((c[i]i))`.

При `STYXRPC_OK` вывод содержит до восьми различных маршрутов модуля из
листов результата с тегом 31. Функция сверяет весь граф значений с объявленной
схемой, объединяет одинаковые алиасы и отклоняет неверные или противоречивые
листы. При `STYXRPC_INVALID` прежний `routes` сохраняется. Берите поле из
успешного ответа на fid функции и держите этот fid открытым на время работы с
каналами. Результат хранит сетевые адреса. Ввода-вывода и выделения памяти нет.
Подключите `StyxRpc::rpc`.

[styxrpc_channel_element_routes](channel_element_routes.ru.md) · [Пример](../examples/disvm/routed_client.c) · [Исходник](../src/channel_route.c)
