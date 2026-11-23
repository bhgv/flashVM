# styxrpc_graph_request

[English](graph_request.md) · [libstyxrpc](../README.ru.md)

## Синтаксис

```c
#include <styxrpc/graph.h>
StyxRpcStatus styxrpc_graph_request(const StyxRpcSchema *schema, const void *payload,
    size_t size, void *bytes, size_t capacity, size_t *used);
```

Проверяет дочерние записи аргументов и пишет полный кадр с тегом 112. Память принадлежит вызывающему, запрос хранится до проверки ответа.

C99. `StyxRpc::rpc`.

[Пример](../examples/graph_device.c) · [Исходник](../src/graph.c)
