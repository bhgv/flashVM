# styxrpc_terminal_send

[Русский](terminal_send.ru.md) · [unibus_terminal.h](../include/styxrpc/unibus_terminal.h) · [unibus_terminal.c](../src/unibus_terminal.c)

`StyxRpc::rpc`

```c
StyxRpcStatus styxrpc_terminal_send(StyxRpcTerminal *terminal, const void *bytes, size_t size);
```

Offer size counted bytes while the peer has granted credit. OK copies bytes and spends the credit. MORE permits retry without acceptance. INVALID reports a malformed frame or failed port. Owner only. No network I/O is done here.

[StyxRpcTerminal](unibus_terminal.md)
