# styxrpc_channel_pid_import_until

[Русский](channel_pid_import_until.ru.md) · [Declaration](../include/styxrpc/channel_route.h#L120) · [Implementation](../src/channel_pid.c#L89)

Admit reverse PID channels from one typed element and retain their stable indices.

## Syntax

```c
#include <styxrpc/channel_route.h>

StyxRpcStatus styxrpc_channel_pid_import_until(struct StyxClient *reverse, const char *shape,
    const void *bytes, size_t size, StyxRpcChannelRoutes *routes, StyxRpcCallStop stop, void *state);
```

## Availability

Hosted C SDK, linking `StyxRpc::launch`. Verified with native DisVM on Linux64.

## Parameters

| Name | Description |
| --- | --- |
| `reverse` | Borrowed attached Styx client reaching the original issuer's reverse program tree. Keep it alive through the call and subsequent exchanges. |
| `shape` | Borrowed NUL-terminated, parenthesized schema for one value containing channels, for example `(c[i])` or `((c[i]c[s]))`. Channel leaves can be nil or PID grants. Managed references and hybrid handles are outside this profile. |
| `bytes` | Borrowed complete UniBus channel element frame including its tag. |
| `size` | Frame length in bytes, greater than zero and at most `STYXRPC_MANAGED_LIMIT`. |
| `routes` | Caller-owned table. Zero-initialize it for the first import. Reuse it with the same result family and reverse attachment. |
| `stop` | Optional callback. A nonzero result requests stop. NULL uses ordinary blocking waits. |
| `state` | Borrowed context for `stop` through return. NULL is valid if the callback accepts it. |

## Return value

| Code | Condition |
| --- | --- |
| `STYXRPC_OK` | Publishes the complete table after confirming every route. |
| `STYXRPC_INVALID` | Invalid frame, conflicting route, exhausted table, peer refusal or transport failure. |
| `STYXRPC_CANCELLED` | Stop was observed before publication. Preserves the table and all previous indices. |

## Behavior

For each distinct PID, the SDK walks to `.prog/<pid>`, opens `.pid` for read, matches its entire decimal line including the newline, and checks that `.unibus` resolves under the same directory. It then retires the temporary fids. One root per PID is stored with its original key, schema and rendezvous flag. Repeated identical routes reuse their indices.

The caller associates the frame with its original result and serializes use of the table or lease. Import outside reverse dispatcher callbacks. The stop hook runs on the calling thread. It should return promptly and avoid reentering the session. Multiplexed clients can flush an outstanding request by tag. Sequential clients sample stop between exchanges. After cancellation, cleanup of temporary fids can finish after return. After refusal or stop, previously admitted indices can still be used for new exchanges.

After import, a [PID exchange](channel_pid_exchange_until.md) uses a known root and can discover current-bus child keys. The issuing peer can revoke a route after import.

## Example

Initialize the table once per result and mount. The tutorial supplies the callback and deadline.

```c
StyxRpcChannelRoutes routes = {0};
StyxRpcStatus result = styxrpc_channel_pid_import_until(reverse, "(c[i])",
    frame, frame_size, &routes, deadline_reached, &deadline);
```

## See also

[`styxrpc_channel_pid_import`](channel_pid_import.md) · [Runnable example](../examples/disvm/CHANNEL_ADMISSION.md)
