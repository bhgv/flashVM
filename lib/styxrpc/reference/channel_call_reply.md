# styxrpc_channel_call_reply

[Русский](channel_call_reply.ru.md) · [libstyxrpc](../README.md)

## Syntax

```c
#include <styxrpc/channel_call.h>
StyxRpcStatus styxrpc_channel_call_reply(const StyxRpcSchema *schema, const StyxRpcChannelCall *call,
    const void *result, size_t result_size, const void *updates, size_t update_size,
    void *bytes, size_t capacity, size_t *used);
```

Borrow the original checked `call` and its exact `schema`. Pass the complete result record and the complete tag 19 updates record with their byte lengths. An empty update list is a valid tag 19 record. Result and update inputs are disjoint from destination `bytes`. `capacity` is the actual writable extent and the encoded reply is limited to 65,536 bytes. On `STYXRPC_OK`, `used` is the complete reply length. `STYXRPC_INVALID` rejects an incompatible schema, invalid ref identity or channel shape, malformed or inconsistent changes, or inadequate space. A supplied `used` is zeroed on refusal. The function performs local validation and encoding without I/O.

`StyxRpc::rpc`.

[Example](../examples/disvm/channel_device.c) · [Source](../src/channel_call.c)
