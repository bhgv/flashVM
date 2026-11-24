# styxrpc_terminal_allow

[Русский](terminal_allow.ru.md) · [unibus_terminal.h](../include/styxrpc/unibus_terminal.h) · [unibus_terminal.c](../src/unibus_terminal.c)

`StyxRpc::rpc`

```c
StyxRpcStatus styxrpc_terminal_allow(StyxRpcTerminal *terminal);
```

Queue one empty credit on terminal’s live port. OK records the grant, MORE means no free slot or no capacity. INVALID reports a port failure. Call on the owner. No network I/O is done here.

[StyxRpcTerminal](unibus_terminal.md)
