# styxrpc_node_service_close

[English](node_service_close.md) · [Объявление](../include/styxrpc/node_service.h#L39) · [Реализация](../src/node_service.c#L153) · [Обмен по Styx](node_exec.ru.md)

```c
#include <styxrpc/node_service.h>
void styxrpc_node_service_close(StyxRpcNodeService *file);
```

Подключите StyxRpc::rpc. Сериализуйте операции на одном fid у владельца транспорта.

Освободите собственный курсор вне одновременных операций I/O и вызова колбэка. Буфер запроса и ответ освобождаются. NULL допустим. Закрытие неполного запроса не запускает колбэк. Контекст остаётся у хоста.

```c
styxrpc_node_service_close(fid);
```
