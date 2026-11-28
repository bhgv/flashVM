# styxrpc_channel_mailbox_result_create

[Русский](channel_mailbox_result_create.ru.md) · [Declaration](../include/styxrpc/channel_mailbox.h#L39) · [Implementation](../src/channel_mailbox.c#L251)

```c
#include <styxrpc/channel_mailbox.h>
StyxRpcStatus styxrpc_channel_mailbox_result_create(const char *shape, unsigned slots,
    StyxRpcChannelMailbox **mailbox, StyxRpcChannelOutput *output);
```

Link `StyxRpc::serve`.

Create a typed module result with two independent directions. `shape` is a NUL-terminated parenthesized type for one copied value or a value carrying PID channel grants. `slots` is from 0 through 8 per direction, with zero for rendezvous. The authenticated resource reads replies at offset zero and handles one guard per fid.

On `STYXRPC_OK`, `output` holds the resource and `mailbox` owns a separate reference. Submit the resource as a [leased module result](channel_handlers_create_with_results.md). The module retires all fids and calls the resource release callback on clunk. The owner can [send](channel_mailbox_send.md) and [receive](channel_mailbox_receive.md) during publication. After retirement both calls return `STYXRPC_INVALID`. Release the separate owner reference with [owner release](channel_mailbox_owner_release.md). Either reference can retire first. Invalid arguments and allocation failure return `STYXRPC_INVALID` and preserve both outputs. [C tests](../tests/channel_mailbox.c)

[Runnable C, Limbo and AS2 example](../examples/disvm/CHANNEL_RESULTS.md).
