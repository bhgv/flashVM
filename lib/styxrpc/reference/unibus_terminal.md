# UniBus terminal frames

[Русский](unibus_terminal.ru.md) · [unibus_terminal.h](../include/styxrpc/unibus_terminal.h)

A `StyxRpcTerminal` uses one open message port with two schemas.
An empty `_inferno.credit` record at version 1 permits one incoming
`_inferno.fvd1` frame at version 1. A frame starts with an fvd1 header
and is 53–1077 bytes. The application decodes its contents. The owner
takes the copied bytes from `terminal.bytes`, clears `terminal.size`,
and issues the next credit. A duplicate credit before send or a
frame without a grant fails the port.

Zero the structure, configure [types](terminal_types.md) and close
the old port before reusing the structure. Store the exact open port
in `terminal.port`, [grant credit](terminal_allow.md) and
[send frames](terminal_send.md).

[Native pair test](../tests/flashvm/native_terminal.b)
