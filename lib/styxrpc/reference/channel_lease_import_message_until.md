# styxrpc_channel_lease_import_message_until

[Русский](channel_lease_import_message_until.ru.md) · [Declaration](../include/styxrpc/channel_client.h#L92) · [Implementation](../src/channel_client.c#L345)

Authenticate delegated module tokens and admit their routes on the result's pinned module.

## Syntax

```c
#include <styxrpc/channel_client.h>

StyxRpcStatus styxrpc_channel_lease_import_message_until(StyxRpcChannelLease *lease, const char *shape,
    const void *bytes, size_t size, StyxRpcCallStop stop, void *state);
```

## Availability

Hosted C SDK, linking `StyxRpc::launch`. Verified with native DisVM on Linux64.

## Parameters

| Name | Description |
| --- | --- |
| `lease` | Borrowed live issuing lease. Keep one owned reference while importing and using its admitted routes. |
| `shape` | Borrowed NUL-terminated, parenthesized schema for one value containing channels, for example `(c[c[i]])`. Channel leaves can be nil or module grants. Managed references and hybrid handles are outside this profile. |
| `bytes` | Borrowed complete UniBus channel element frame delivered for this result family. |
| `size` | Frame length in bytes, greater than zero and at most `STYXRPC_MANAGED_LIMIT`. |
| `stop` | Optional callback. A nonzero result requests stop. NULL uses ordinary blocking waits. |
| `state` | Borrowed context for `stop` through return. NULL is valid if the callback accepts it. |

## Return value

| Code | Condition |
| --- | --- |
| `STYXRPC_OK` | Publishes the complete table after confirming every route. |
| `STYXRPC_INVALID` | Invalid frame, conflicting route, exhausted table, peer refusal or transport failure. |
| `STYXRPC_CANCELLED` | Stop was observed before publication. Preserves the table and all previous indices. |

## Behavior

For each distinct token, the SDK walks `.unibus` from the module pinned by the lease, opens a temporary read/write fid, writes the whole 16-byte token at offset zero, and closes that fid. The original key, element schema and rendezvous flag remain associated with the checked token. Admission sends only the authentication write on each temporary fid.

The caller associates the frame with its original result and serializes use of the table or lease. Import outside reverse dispatcher callbacks. The stop hook runs on the calling thread. It should return promptly and avoid reentering the session. Multiplexed clients can flush an outstanding request by tag. Sequential clients sample stop between exchanges. After cancellation, cleanup of temporary fids can finish after return. After refusal or stop, previously admitted indices can still be used for new exchanges.

Find the resulting capabilities with [lease route](channel_lease_route.md). A [leased exchange](channel_lease_exchange_until.md) selects guards on a separate fid. The issuer can retire tokens by clunking their originating result.

## Example

Pass a frame that the host has already associated with the original result. The tutorial supplies the callback and deadline.

```c
StyxRpcStatus result = styxrpc_channel_lease_import_message_until(lease,
    "(c[c[i]])", frame, frame_size, deadline_reached, &deadline);
```

## See also

[`styxrpc_channel_lease_import_message`](channel_lease_import_message.md) · [Runnable example](../examples/disvm/CHANNEL_ADMISSION.md)
