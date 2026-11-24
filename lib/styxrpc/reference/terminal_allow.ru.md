# styxrpc_terminal_allow

[English](terminal_allow.md) · [unibus_terminal.h](../include/styxrpc/unibus_terminal.h) · [unibus_terminal.c](../src/unibus_terminal.c)

`StyxRpc::rpc`

```c
StyxRpcStatus styxrpc_terminal_allow(StyxRpcTerminal *terminal);
```

Поставить одно пустое разрешение в очередь живого порта terminal. OK фиксирует разрешение, MORE означает занятое место или нехватку ёмкости. INVALID сообщает отказ порта. Вызывайте у владельца. Сетевые операции здесь отсутствуют.

[StyxRpcTerminal](unibus_terminal.ru.md)
