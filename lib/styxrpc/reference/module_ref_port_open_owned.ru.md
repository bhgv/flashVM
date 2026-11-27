# styxrpc_module_ref_port_open_owned

[English](module_ref_port_open_owned.md) · [Объявление](../include/styxrpc/module_ref_port.h) · [Реализация](../src/module_ref_port.c) · [Руководство](module_ref_port.ru.md)

`StyxRpc::launch`

```c
StyxRpcModuleRefPort *styxrpc_module_ref_port_open_owned(struct StyxClient *issuer, unsigned owner,
    const void *scope, StyxRpcModuleRefLocalOpen local, StyxRpcModuleRefLocalCapture capture, void *context);
```

Передайте обычное подключение в `issuer`, его прямую или обратную роль
в `owner` и согласованные 16 байтов в `scope`. Порт удерживает клиента,
копирует область и заимствует оба колбэка и их `context` до
[закрытия](module_ref_port_close.ru.md).

Колбэк `local` разрешает принятый дескриптор, как при
[парном открытии](module_ref_port_open_paired.ru.md). Ненулевой
`capture` требует `local` и включает команду первой записи
`STYXRPC_REF_PORT_CAPTURE_LOCAL`. Колбэк захвата получает канонический
путь, выбранный режим и противоположную роль владельца в `ref`.
Он открывает точный местный экспорт, записывает его обычный выданный
Qid в `ref->node` и передаёт файл в обнулённый результат. При отказе
результат остаётся пустым. NULL в `capture` отключает местный захват.
Для дальнейших чтения и записи используйте обычные операции
[позиционного порта](module_ref_port.ru.md).

Вызов возвращает собственный пустой порт либо NULL при неверных
аргументах, отказе выделения памяти или удержания. При создании порт
не обращается к транспорту. При местном захвате колбэк может ждать
провайдера. Выполняйте операции на каждом порту последовательно.
