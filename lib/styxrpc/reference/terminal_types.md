# styxrpc_terminal_types

[Русский](terminal_types.ru.md) · [unibus_terminal.h](../include/styxrpc/unibus_terminal.h) · [unibus_terminal.c](../src/unibus_terminal.c)

`StyxRpc::rpc`

```c
void styxrpc_terminal_types(StyxRpcTerminal *terminal, StyxRpcMessageType *types);
```

Fill two writable entries in types. Open a port with both entries and retain terminal through close. Configure from the port owner before starting transfers.

[StyxRpcTerminal](unibus_terminal.md)
