# styxrpc_graph_profile

[English](graph_profile.md) · [libstyxrpc](../README.ru.md)

## Синтаксис

```c
#include <styxrpc/graph.h>
StyxRpcStatus styxrpc_graph_profile(const char *name, uint32_t signature, const StyxRpcSchema *schema,
    void *bytes, size_t capacity, size_t *used);
```

Создаёт одну запись .interface по точной скомпилированной сигнатуре. Опубликуйте позиционный маркер в .ref-fields каталога модуля. Этот формат значения не передаёт полномочия канала.

C99. `StyxRpc::rpc`.

[Пример](../examples/graph_device.c) · [Исходник](../src/graph.c)
