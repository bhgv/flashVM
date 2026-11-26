# styxrpc_node_service_write

[English](node_service_write.md) · [Объявление](../include/styxrpc/node_service.h#L27) · [Реализация](../src/node_service.c#L76) · [Обмен по Styx](node_exec.ru.md)

```c
#include <styxrpc/node_service.h>
int styxrpc_node_service_write(StyxRpcNodeService *file, uint64_t offset,
    const void *input, size_t size, size_t *used);
```

Подключите StyxRpc::rpc. Сериализуйте операции на одном fid у владельца транспорта.

Передайте непустой заимствованный фрагмент по следующему смещению, начиная с нуля. При успехе возвращается 1, а used содержит длину принятого фрагмента. Последний фрагмент синхронно запускает колбэк один раз. Ошибка формата или отказ обработчика дают 0. После ошибки закройте курсор. Курсор, контекст и вход остаются живыми до возврата.

```c
ok = styxrpc_node_service_write(fid, offset, bytes, size, &used);
```
