# styxrpc_node_service_open

[English](node_service_open.md) · [Объявление](../include/styxrpc/node_service.h#L20) · [Реализация](../src/node_service.c#L62) · [Обмен по Styx](node_exec.ru.md)

```c
#include <styxrpc/node_service.h>
StyxRpcNodeService *styxrpc_node_service_open(StyxRpcNodeInvoke invoke, void *context);
```

Подключите StyxRpc::rpc. Сериализуйте операции на одном fid у владельца транспорта.

Создайте отдельный курсор исполнения до приёма данных. Вызывающий владеет результатом и закрывает его при clunk. Курсор заимствует invoke и context. Колбэк получает относительное имя с завершающим нулём, изменяемый буфер, его размер и выходную длину. Верните из колбэка 1 с длиной до размера буфера либо 0 при отказе. При отсутствии колбэка или памяти возвращается NULL.

```c
StyxRpcNodeService *fid = styxrpc_node_service_open(invoke, context);
```
