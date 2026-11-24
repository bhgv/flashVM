# styxrpc_terminal_types

[English](terminal_types.md) · [unibus_terminal.h](../include/styxrpc/unibus_terminal.h) · [unibus_terminal.c](../src/unibus_terminal.c)

`StyxRpc::rpc`

```c
void styxrpc_terminal_types(StyxRpcTerminal *terminal, StyxRpcMessageType *types);
```

Заполнить два доступных для записи элемента types. Открыть порт с обоими типами и удерживать terminal до закрытия. Настроить из потока владельца до обмена.

[StyxRpcTerminal](unibus_terminal.ru.md)
