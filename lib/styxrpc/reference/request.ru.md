# styxrpc_request

[English](request.md) · [libstyxrpc](../README.ru.md)

## Синтаксис

```c
#include <styxrpc/rpc.h>
StyxRpcStatus styxrpc_request(uint32_t signature, int64_t argument, void *bytes, size_t capacity, size_t *size);
```

Кодирует один знаковый argument с сигнатурой signature в bytes. capacity и size измеряются в байтах. Память выделяет вызывающий. При ошибке size равен нулю.

C99. Вызывайте из диспетчера, сериализующего операции одного fid. `StyxRpc::rpc`.

[Пример](../examples/client.c) · [Исходник](../src/rpc.c)
