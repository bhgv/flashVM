# styxrpc_channel_element_remap

[Русский](channel_element_remap.ru.md) · [Declaration](../include/styxrpc/channel_route.h) · [Implementation](../src/channel_remap.c) · [Scope guide](channel_argument_scopes.md)

Include `<styxrpc/channel_route.h>` and link `StyxRpc::rpc`.

```c
StyxRpcStatus styxrpc_channel_element_remap(const char *shape, const void *bytes, size_t size,
    const StyxRpcChannelKeyMap *keys, size_t count, void *output, size_t capacity, size_t *used);
```

| Parameter | Contract |
| --- | --- |
| `shape` | Exact terminated, parenthesized type list for the complete channel element. |
| `bytes`, `size` | One validated typed element to copy, up to `STYXRPC_MANAGED_LIMIT` bytes. |
| `keys`, `count` | Zero to eight source–destination pairs of positive 31-bit keys. Both columns are unique. Each current-bus source in the element requires a mapping. An empty map can pass NULL. |
| `output`, `capacity`, `used` | Disjoint destination storage, its capacity up to `STYXRPC_MANAGED_LIMIT`, and the exact byte count returned on success. |

Replaces channel leaf keys with ordinary scoped keys and reconstructs enclosing record lengths and checksums. Nil and ordinary fields retain their values. `STYXRPC_OK` indicates a complete frame. Bad frames or mappings, separately addressed routes, allocation failure and insufficient capacity return `STYXRPC_INVALID`. This operation performs no I/O and does not create a grant. [Stage the new keys](argument_scope_nested.md) on the intended caller bus before activation. [Example](../tests/channel_arguments.c)
