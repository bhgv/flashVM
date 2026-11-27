# styxrpc_module_ref_port_read

[English](module_ref_port_read.md) · [Объявление](../include/styxrpc/module_ref_port.h#L29) · [Реализация](../src/module_ref_port.c#L109) · [Руководство](module_ref_port.ru.md)

`StyxRpc::launch`

```c
StyxRpcIoOutcome styxrpc_module_ref_port_read(StyxRpcModuleRefPort *port,
    uint64_t offset, void *bytes, size_t size, size_t *used);
```

Передайте привязанный `port`, выходной буфер `bytes` на `size` байтов и адрес счётчика `used`. При `offset` ноль запрашивайте ровно четыре байта знакового Int32. Вызов может открыть отложенный fid и ждать владельца. Со смещения четыре и далее копируется сохранённый дескриптор. Возможно короткое чтение или ноль на EOF. Этот режим не выполняет запросов Styx.

`STYXRPC_IO_OK` подтверждает число байтов. Пустой порт, неверный аргумент, недопустимые позиция или размер дают `STYXRPC_IO_NOT_OPEN`. Ошибки обычного подключения передаются вызывающему. Короткое чтение значения даёт `STYXRPC_IO_FAILED`. Если `used` задан, счётчик сначала обнуляется. Буферы заимствуются. Используйте порт последовательно.

При пересылке начинайте с `STYXRPC_REF_PORT_FRAME` и передавайте полную последовательность байтов по тому же именованному каналу.
