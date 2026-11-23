# styxrpc_channel_lease_import_message

[Русский](channel_lease_import_message.ru.md) · [Lease guide](channel_leases.md) · [Declaration](../include/styxrpc/channel_client.h#L70) · [Implementation](../src/channel_client.c#L284)

```c
#include <styxrpc/channel_client.h>
StyxRpcStatus styxrpc_channel_lease_import_message(StyxRpcChannelLease *lease,
    const char *shape, const void *bytes, size_t size);
```

Link `StyxRpc::launch`. On the owner of `lease`, supply one typed element from a reverse endpoint that your application associated with that exact result family. Give the NUL-terminated element schema in `shape`, such as `(c[c[i]])`, the complete UniBus element frame in `bytes`, and its byte length in `size` up to `STYXRPC_MANAGED_LIMIT`. The frame and shape are borrowed for the call. The profile accepts module grants and nil channel leaves. Managed references and hybrid handles are outside this profile.

The SDK validates the entire frame and reserves route indices in a temporary table. For each distinct token it walks `.unibus` from the pinned module, opens a temporary read/write fid, writes the whole token at offset zero, and clunks the fid. This confirms the token on that module without sending a channel operation. The key, element type and rendezvous flag come from the typed message. The reverse endpoint provides the message's association with the original result, and that association is the caller's responsibility.

`STYXRPC_OK` commits the staged table. Find its indices with [lease route](channel_lease_route.md) and use [leased exchange](channel_lease_exchange.md). `STYXRPC_INVALID` leaves the table unchanged on malformed input, conflicting or unsupported grants, exhausted slots, or authentication and transport failure. Each Styx operation uses the mounted client's timeout. Call outside reverse dispatcher callbacks and keep the issuing lease alive through the call. The peer may later revoke a token, so subsequent operations still authenticate it on their own fids.

[Native integration fixture](../tests/flashvm/run.py) · [Import same-bus keys](channel_lease_import_element.md)
