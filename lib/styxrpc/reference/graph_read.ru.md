# styxrpc_graph_read

[English](graph_read.md) · [libstyxrpc](../README.ru.md)

## Синтаксис

```c
#include <styxrpc/graph.h>
StyxRpcStatus styxrpc_graph_read(const StyxRpcSchema *schema, const void *request, size_t request_size,
    const void *reply, size_t reply_size, StyxRpcGraphReply *decoded);
```

Проверяет оба полных кадра по схеме и выдаёт заимствованные результат и список изменений. REMOTE_ERROR означает сообщение об ошибке провайдера. Приложение применяет проверенные изменения к своим объектам.

C99. `StyxRpc::rpc`.

[Пример](../examples/graph_device.c) · [Исходник](../src/graph.c)
