# styxrpc_channel_element_routes

[Русский](channel_element_routes.ru.md) · [libstyxrpc](../README.md)

## Syntax

```c
#include <styxrpc/channel_route.h>
StyxRpcStatus styxrpc_channel_element_routes(const char *shape, const void *bytes, size_t size,
    StyxRpcChannelRoutes *routes);
```

Read one complete typed element record (tag 110). `shape` is its declared
element schema in parentheses, for example `(c[i])` for an element of type
`chan of int`. Pass the received bytes and their exact length as `bytes` and
`size`. Keep them alive through the call. `routes` receives a copy of at most
eight distinct non-nil addresses, in first-occurrence order.

The three kinds are CURRENT, PID and MODULE. All carry a positive 31-bit key,
one declared element schema and a rendezvous flag. A PID in the delegated
namespace has its top bit set, and a module address carries a nonzero 16-byte
token. Equal aliases coalesce. An inconsistent key, capacity flag or element
type for the same identity is rejected. A graph with no channels returns an
empty table.

Returns `STYXRPC_OK` on success. `STYXRPC_INVALID` leaves `routes` untouched
when input is incomplete, too large, malformed or has too many or conflicting
addresses. The codec performs no I/O and borrows no result storage. Grant
authority comes from the original authenticated route and its lifetime.
Link `StyxRpc::rpc`.

[styxrpc_channel_route_value](channel_route_value.md) · [Example](../examples/disvm/routed_client.c) · [Source](../src/channel_route.c)
