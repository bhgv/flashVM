# styxrpc_terminal_send

[English](terminal_send.md) · [unibus_terminal.h](../include/styxrpc/unibus_terminal.h) · [unibus_terminal.c](../src/unibus_terminal.c)

`StyxRpc::rpc`

```c
StyxRpcStatus styxrpc_terminal_send(StyxRpcTerminal *terminal, const void *bytes, size_t size);
```

Предложить size байт в bytes при наличии разрешения получателя. OK копирует байты и расходует разрешение. MORE разрешает повтор без приёма. INVALID сообщает неверный кадр либо отказ порта. Только владелец. Сетевые операции здесь отсутствуют.

[StyxRpcTerminal](unibus_terminal.ru.md)
