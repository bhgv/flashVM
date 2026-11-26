# styxrpc_node_service_read

[English](node_service_read.md) · [Объявление](../include/styxrpc/node_service.h#L33) · [Реализация](../src/node_service.c#L134) · [Обмен по Styx](node_exec.ru.md)

```c
#include <styxrpc/node_service.h>
int styxrpc_node_service_read(StyxRpcNodeService *file, uint64_t offset,
    void *bytes, size_t size, size_t *used);
```

Подключите StyxRpc::rpc. Сериализуйте операции на одном fid у владельца транспорта.

Скопируйте до size подтверждённых байтов ответа со смещения offset в буфер вызывающего. До успешного завершения возвращается 0. После завершения возвращается 1, а used содержит число скопированных байтов либо ноль на EOF. Повторное и позиционное чтение используют тот же сохранённый ответ.

```c
ok = styxrpc_node_service_read(fid, offset, bytes, size, &used);
```
