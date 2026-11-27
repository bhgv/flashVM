# styxrpc_channel_exchange

[Русский](channel_exchange.ru.md) · [libstyxrpc](../README.md)

## Syntax

```c
#include <styxrpc/channel_call.h>
StyxRpcStatus styxrpc_channel_exchange(struct StyxClient *reverse, const StyxRpcChannelCall *call,
    const StyxRpcSelection *selection, unsigned timeout_ms, void *scratch, size_t scratch_size,
    void *request, size_t request_capacity, void *reply, size_t reply_capacity, StyxRpcChannelResult *result);
```

Execute a selection on the checked caller’s reverse .unibus using its admitted keys. Open and close one fid per selection. Give disjoint buffers and a timeout in milliseconds (1–60000). MORE means timeout with a clunk attempt. INVALID on link or protocol failure and REMOTE_ERROR on explicit refusal. Treat timeout and lost acknowledgements as uncertain completion. The forward dispatcher must remain responsive while DisVM services reverse operations.

`StyxRpc::launch`.

[Example](../examples/disvm/channel_device.c) · [Source](../src/channel_exchange.c)

[Cancellable operation](channel_exchange_until.md).
