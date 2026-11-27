# styxrpc_module_ref_port_close

[English](module_ref_port_close.md) · [Объявление](../include/styxrpc/module_ref_port.h#L58) · [Реализация](../src/module_ref_port.c#L225) · [Руководство](module_ref_port.ru.md)

`StyxRpc::launch`

```c
void styxrpc_module_ref_port_close(StyxRpcModuleRefPort *port);
```

Передайте собственный `port` от [открытия](module_ref_port_open.ru.md) или [парного открытия](module_ref_port_open_paired.ru.md) либо NULL. Сначала завершите операции с ним. Вызов освобождает дескриптор и удерживаемую ссылку на клиента. Открытый удалённый fid ставится в очередь на clunk без синхронного запроса Styx. Местный файл закрывается через его операции VFS. Другие дескрипторы удерживают свои подключения.

Закройте порт после чтения итогового значения или после передачи дескриптора другому участнику, когда эта привязка больше не нужна.
