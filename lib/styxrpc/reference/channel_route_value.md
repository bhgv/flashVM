# styxrpc_channel_route_value

[Русский](channel_route_value.ru.md) · [libstyxrpc](../README.md)

## Syntax

```c
#include <styxrpc/channel_route.h>
StyxRpcStatus styxrpc_channel_route_value(const StyxRpcChannelRoute *route, void *bytes,
    size_t capacity, size_t *used);
```

Write one nested channel leaf. `route` contains the exact declared element
schema, key and capacity flag. Provide the PID with a high bit for a PID
route, the nonzero 16-byte token for a MODULE route, or neither for CURRENT.
All unused address fields must be zero. `bytes` is caller-provided storage
with `capacity` bytes.

Returns `STYXRPC_OK` and sets `*used` to the complete leaf length. On
`STYXRPC_INVALID` the descriptor or output is invalid and `*used` is zero
when supplied. Put the leaf inside the enclosing typed element. The wire tag
is 33 for PID, 35 for MODULE, or 24/34 for a regular/synchronous CURRENT key.
Address encoding performs no publication. The application publishes the
resource and holds its lifetime through pending operations. Link
`StyxRpc::rpc`.

[styxrpc_channel_read_routed](channel_read_routed.md) · [Example](../examples/disvm/routed_client.c) · [Source](../src/channel_route.c)
