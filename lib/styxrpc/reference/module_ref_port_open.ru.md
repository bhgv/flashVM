# styxrpc_module_ref_port_open

[English](module_ref_port_open.md) · [Объявление](../include/styxrpc/module_ref_port.h#L22) · [Реализация](../src/module_ref_port.c#L26) · [Руководство](module_ref_port.ru.md)

`StyxRpc::launch`

```c
StyxRpcModuleRefPort *styxrpc_module_ref_port_open(struct StyxClient *issuer,
    unsigned owner, const void *scope);
```

Передайте обычное выданное подключение к VFS в `issuer`. В `owner` укажите `STYXRPC_MODULE_REF_FORWARD` или `STYXRPC_MODULE_REF_REVERSE` для публикации этого клиента. Передайте согласованные 16 байтов в `scope`. Порт копирует их и удерживает собственную ссылку на клиента. Вызов не выполняет запросов Styx.

При успехе возвращается собственный пустой порт. Неверные аргументы, отказ выделения памяти или удержания клиента дают NULL. Привяжите порт одной [записью](module_ref_port_write.ru.md), затем используйте [чтение](module_ref_port_read.ru.md) и [закрытие](module_ref_port_close.ru.md). В штатной паре с DisVM клиент указывает на FlashVM стороны forward. Используйте каждый порт последовательно.
