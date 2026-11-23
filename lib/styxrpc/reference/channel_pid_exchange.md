# styxrpc_channel_pid_exchange

[Русский](channel_pid_exchange.ru.md) · [Declaration](../include/styxrpc/channel_route.h#L112) · [Implementation](../src/channel_pid.c#L148)

```c
#include <styxrpc/channel_route.h>
StyxRpcStatus styxrpc_channel_pid_exchange(struct StyxClient *reverse,
    StyxRpcChannelRoutes *routes, size_t anchor, const StyxRpcSelection *selection,
    unsigned timeout_ms, void *scratch, size_t scratch_size,
    void *request, size_t request_capacity, void *reply, size_t reply_capacity,
    StyxRpcChannelResult *result);
```

Link `StyxRpc::launch`. This synchronous operation uses the native reverse Styx and UniBus profile. It can send, receive or select on several guards on one PID and learn channel keys from a received element.

## Set up the exchange

- Pass the same borrowed `reverse` mount and caller-owned `routes` table used for [PID import](channel_pid_import.md). Keep the result lease and mount live, and serialize access to the table and mount. The zero-based `anchor` can be the index of any admitted channel on the target PID.
- Use a `selection` containing one or more typed guards whose keys are already known on this PID. Set `selection->policy` to NULL. For a single guard use `STYXRPC_CHANNEL_ONE`. For a blocking or immediate choice use `STYXRPC_CHANNEL_ALT` or `STYXRPC_CHANNEL_TRY`. A send may contain nil and known channel keys encoded as ordinary CURRENT leaves, scoped to this PID.
- Pass a budget of 1 through 60000 milliseconds in `timeout_ms`. The adapter checks the budget between synchronous Styx transactions. Each Styx transaction also uses the timeout of the mounted client.
- Supply a distinct writable `request` buffer and its byte capacity for the encoded selection. Supply a distinct writable `reply` buffer sized in bytes up to `STYXRPC_MANAGED_LIMIT` for the complete reply. For ALT and TRY, supply separate writable `scratch` storage and its byte size for the guards before framing. For ONE pass NULL and zero for scratch. Keep all buffers and selection data active through the call.
- Provide writable `result` storage. On success `result->selected` is the one-based guard ordinal, or zero for an idle TRY. A received value points into `reply` and remains valid until that buffer is reused.

## Outcomes and lifetime

On `STYXRPC_OK`, each CURRENT child from a received element is recorded under its PID and key with its schema and capacity flag. Aliases reuse stable indices. The table has room for eight entries total, counting roots and children across all admitted PIDs. Any external address, conflicting child or exhausted table rejects the update as a whole. A child uses the PID's lifetime and can be used independently of its parent index.

The temporary operation fid is clunked before the call returns. `STYXRPC_MORE` reports an elapsed budget, and `STYXRPC_REMOTE_ERROR` reports an acknowledged channel error. `STYXRPC_INVALID` covers invalid arguments, a key outside the selected PID, transport failure, a revoked PID or an invalid result. On failure `routes` and `result` retain their previous values. A remote operation can complete before a timeout or local rejection. Automatic retries can duplicate the operation.

The [independent socket test](../tests/channel_socket.c) imports a string-channel PID and receives with its admitted key. It then tries a second key that the peer serves on that bus. The adapter rejects the second request because that key has yet to be admitted. The [lease guide](channel_leases.md) describes how the owner obtains the PID frame in the first place.
