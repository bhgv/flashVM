# styxrpc_messages_flush

[Русский](messages_flush.ru.md) · [Header](../include/styxrpc/messages.h) · [Implementation](../src/message_client.c)

```c
#include <styxrpc/messages.h>
StyxRpcStatus styxrpc_messages_flush(StyxRpcMessagePort *port);
```

Link the hosted StyxRpc SDK. Pass a live client port opened on an attached
Styx session. The port remains owned by the caller. The call writes queued
records, including the initial type declaration, without reading or
invoking receive handlers.

`STYXRPC_OK` means Styx acknowledged all queued bytes or the queue was
empty. `STYXRPC_INVALID` means an invalid or unusable port, recursive
operation, or incomplete/failed transport write. A write failure retires
the port. Dispose of it with [styxrpc_messages_close](messages_close.md).
A failed write can have reached the peer. Do not blindly retry it.

Run on the client owner thread outside callbacks on the same port or
session. The write uses the normal Styx request deadlines. To send and
read in one operation use [styxrpc_messages_pump](messages_pump.md).
