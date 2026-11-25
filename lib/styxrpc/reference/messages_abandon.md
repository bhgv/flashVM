# styxrpc_messages_abandon

[Русский](messages_abandon.ru.md) · [Header](../include/styxrpc/messages.h) · [Implementation](../src/message_client.c) · [Close](messages_close.md)

```c
#include <styxrpc/messages.h>
void styxrpc_messages_abandon(StyxRpcMessagePort *port);
```

Dispose of an idle client message port on its owner and defer the fid clunk to
the attached session. Use this when object reclamation cannot perform transport
I/O. The session must outlive the call. Queued bytes and registered names are
released immediately. A null port is allowed. Call outside pump and receive
callbacks, then discard the pointer. Use
[styxrpc_messages_close](messages_close.md) for synchronous cleanup.
