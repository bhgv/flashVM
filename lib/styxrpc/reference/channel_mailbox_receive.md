# styxrpc_channel_mailbox_receive

[Русский](channel_mailbox_receive.ru.md) · [Declaration](../include/styxrpc/channel_mailbox.h#L31) · [Implementation](../src/channel_mailbox.c#L406)

```c
#include <styxrpc/channel_mailbox.h>
StyxRpcStatus styxrpc_channel_mailbox_receive(StyxRpcChannelMailbox *mailbox,
    void *bytes, size_t capacity, size_t *size);
```

Link `StyxRpc::serve`.

Copy and consume the head frame sent by the peer to a live `mailbox`. Supply a writable `bytes` buffer of `capacity` bytes and a required length output `size`. `STYXRPC_OK` writes the complete frame length and frees the slot. `STYXRPC_MORE` means empty. With invalid arguments or insufficient capacity, `STYXRPC_INVALID` preserves the queue and the length output.

The owner and dispatcher share the mailbox mutex. This call moves bytes and may advance a pending peer write. It never waits for an incoming message or performs network I/O. To use the module channels in an incoming frame, [import the message](channel_lease_import_message.md) under the issuing lease on the owner.

[Standalone C test](../tests/channel_mailbox.c)

For `(i)` receive the ordinary Int32 record. With zero slots, `capacity` must be at least `STYXRPC_MANAGED_LIMIT`. A smaller buffer is refused before a sender is matched.
