# styxrpc_graph_arguments

[English](graph_arguments.md) · [libstyxrpc](../README.ru.md)

## Синтаксис

```c
#include <styxrpc/graph.h>
StyxRpcStatus styxrpc_graph_arguments(const StyxRpcSchema *schema, const void *request, size_t request_size,
    FVM_SRecord *arguments);
```

Выдаёт заимствованное представление полного позиционного запроса после проверки графа. Сохраняйте буфер до формирования ответа.

C99. `StyxRpc::rpc`.

[Пример](../examples/graph_device.c) · [Исходник](../src/graph.c)
