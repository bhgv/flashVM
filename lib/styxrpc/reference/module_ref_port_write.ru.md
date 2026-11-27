# styxrpc_module_ref_port_write

[English](module_ref_port_write.md) · [Объявление](../include/styxrpc/module_ref_port.h) · [Реализация](../src/module_ref_port.c) · [Руководство](module_ref_port.ru.md)

`StyxRpc::launch`

```c
StyxRpcIoOutcome styxrpc_module_ref_port_write(StyxRpcModuleRefPort *port,
    uint64_t offset, const void *bytes, size_t size);
```

Передайте `offset` ноль и непустой заимствованный буфер `bytes` на `size` байтов. Пустому порту передайте одну полную [команду захвата или приёма](module_ref_port.ru.md). Захват открывает живой экспорт и сохраняет его Qid. Для местного экспорта передайте команду `STYXRPC_REF_PORT_CAPTURE_LOCAL` в том же формате на порту с [колбэком захвата](module_ref_port_open_owned.ru.md). Приём проверяет роль, область и формат и откладывает открытие. После привязки передавайте ровно четыре байта знакового Int32 старшим байтом вперёд для замены записываемого значения.

`STYXRPC_IO_OK` подтверждает полную команду или замену. Недопустимые аргументы, неверный кадр или режим только для чтения дают `STYXRPC_IO_NOT_OPEN`. Ошибка или неопределённый результат Styx передаётся вызывающему. Не повторяйте неопределённую замену автоматически. Захват и работа со значением могут ждать владельца. Записи, чтения и закрытие одного порта выполняются последовательно.

Для захвата записываемого экспорта предварите путь двумя байтами `STYXRPC_REF_PORT_CAPTURE, 1`. Затем прочитайте полный дескриптор со [смещения четыре](module_ref_port_read.ru.md).

[Вариант с отменой](module_ref_port_write_until.ru.md).
