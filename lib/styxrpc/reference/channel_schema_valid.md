# styxrpc_channel_schema_valid

[Русский](channel_schema_valid.ru.md) · [Declaration](../include/styxrpc/channel_call.h#L33) · [Implementation](../src/channel_call.c#L30) · [Guide](../LIVE_VALUES.md)

## Syntax

```c
#include <styxrpc/channel_call.h>
int styxrpc_channel_schema_valid(const StyxRpcSchema *schema);
```

Link `StyxRpc::rpc`.

## Parameter and result

Borrow a complete [`StyxRpcSchema`](../include/styxrpc/managed.h#L32) containing the parenthesized argument and result shapes. Returns 1 for a valid scoped channel schema with at least one channel position, 0 for a null or incompatible schema. The bounded grammar allows copied data, graph refs and a hybrid table in the same call as channels. A channel cannot be a field of a copied ref. A message carried by a channel cannot contain a ref or a hybrid table. A by-value list keeps its value-only element rule.

This is a thread-independent local check. It allocates no fids and admits no routes. Use [`styxrpc_channel_profile`](channel_profile.md) to publish the checked signature and [`styxrpc_channel_invoke`](channel_invoke.md) to check the peer's mounted offer and call it.

## Example

```c
const StyxRpcSchema schema = { "(r[b]c[i])", "((r[b]c[b]))" };
if (styxrpc_channel_schema_valid(&schema)) {
    /* A compiled provider can return the updated ref and an owned channel. */
}
```
