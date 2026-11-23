# styxrpc_channel_mailbox_send

[Русский](channel_mailbox_send.ru.md) · [Declaration](../include/styxrpc/channel_mailbox.h#L26) · [Implementation](../src/channel_mailbox.c#L234)

```c
#include <styxrpc/channel_mailbox.h>
StyxRpcStatus styxrpc_channel_mailbox_send(StyxRpcChannelMailbox *mailbox, const void *bytes, size_t size);
```

Link `StyxRpc::serve`.

Copy `size` bytes from `bytes` into the outbound FIFO of a live `mailbox`. The buffer must contain one complete typed element accepted by the mailbox schema, with module grants or nil in channel fields. The element must fit a native reply. The API checks the frame and borrows the input only during the call. The owner supplies grants authorized for this connection.

Returns `STYXRPC_OK` after queuing or satisfying a waiting peer read, `STYXRPC_MORE` when the FIFO is full, and `STYXRPC_INVALID` for malformed or unsupported input. The call uses the mailbox mutex and performs no network I/O. It may complete an already pending peer read, which the peer can poll on its own fid. Keep the mailbox alive through the call.

[Standalone C test](../tests/channel_mailbox.c)
