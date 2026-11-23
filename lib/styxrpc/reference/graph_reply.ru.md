# styxrpc_graph_reply

[English](graph_reply.md) · [libstyxrpc](../README.ru.md)

## Синтаксис

```c
#include <styxrpc/graph.h>
StyxRpcStatus styxrpc_graph_reply(const StyxRpcSchema *schema, const void *request, size_t request_size,
    const void *result, size_t result_size, const void *updates, size_t update_size,
    void *bytes, size_t capacity, size_t *used);
```

Упаковывает две готовые записи, результат и список изменений. Перед записью сверяет обе по исходному запросу и идентификаторам его ссылок. Выходной буфер отдельный.

C99. `StyxRpc::rpc`.

[Пример](../examples/graph_device.c) · [Исходник](../src/graph.c)
