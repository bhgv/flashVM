# styxrpc_channel_mailbox_create

[Русский](channel_mailbox_create.ru.md) · [Declaration](../include/styxrpc/channel_mailbox.h#L20) · [Implementation](../src/channel_mailbox.c#L241)

```c
#include <styxrpc/channel_mailbox.h>
StyxRpcStatus styxrpc_channel_mailbox_create(const char *shape, unsigned slots,
    StyxRpcChannelMailbox **mailbox, StyxRpcChannelOutput *output);
```

Link `StyxRpc::serve`.

Create a reverse UniBus resource with two independent FIFOs of `slots` each. Choose 1 through 8 slots. `shape` is a NUL-terminated parenthesized schema for one typed element with channel fields, for example `(c[i])`. Its channel leaves carry module grants or nil. The constructor copies the schema. References and hybrid handles are outside this profile.

On `STYXRPC_OK`, `output` owns the resource and `mailbox` borrows it. On invalid arguments or allocation failure the call returns `STYXRPC_INVALID` and preserves both outputs. Register `output.resource` with a [program server](program_server_grant.md). Use the returned PID and `output.key` with the buffered flag. Each opened fid accepts one native guard, with replies read at the request extent.

The owner's [send](channel_mailbox_send.md) and [receive](channel_mailbox_receive.md) work while the dispatcher runs. The server callbacks move copied frames. The host can [verify module tokens](channel_lease_import_message.md) on the relevant result lease when it consumes a frame. Revoke the PID, finish all owner calls, then call `output.release(output.resource.context)`.

[Standalone C test](../tests/channel_mailbox.c)
