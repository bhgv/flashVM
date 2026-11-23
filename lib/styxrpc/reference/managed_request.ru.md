# styxrpc_managed_request

[English](managed_request.md) · [libstyxrpc](../README.ru.md)

## Синтаксис

```c
#include <styxrpc/managed.h>
StyxRpcStatus styxrpc_managed_request(uint32_t signature, const StyxRpcSchema *schema, const void *payload,
    size_t size, void *bytes, size_t capacity, size_t *used);
```

Проверяет size байт дочерних S-записей по сигнатуре и схеме, затем записывает запрос в отдельный буфер bytes. Сохраните used байт для проверки ответа. Ошибка оставляет used равным нулю.

C99. Сериализуйте операции одного fid. `StyxRpc::rpc`.

[Пример](../examples/../tests/managed.c) · [Исходник](../src/managed.c)
