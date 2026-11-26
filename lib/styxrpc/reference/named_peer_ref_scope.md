# styxrpc_named_peer_ref_scope

[Русский](named_peer_ref_scope.ru.md) · [API](../include/styxrpc/named_peer.h#L43) · [Source](../src/named_peer.c#L291) · [Guide](module_refs.md)

`StyxRpc::serve`

```c
int styxrpc_named_peer_ref_scope(StyxRpcNamedPeer *peer, void *scope);
```

## Contract

Call on the forward side with a live [scoped host](named_peer_host_scoped.md) and a writable 16-byte `scope` buffer. Success returns 1 and copies the nonce after the remote named mount advertises support using `STYXRPC_MODULE_REF_ATTACH`. If support is unavailable, or either argument is NULL, it returns 0 and preserves the buffer.

This is a nonblocking check of the peer's current advertised capability. Keep `peer` alive through the call. Use a successful scope for [reference encoding](module_ref_encode.md) and [decoding](module_ref_decode.md).
