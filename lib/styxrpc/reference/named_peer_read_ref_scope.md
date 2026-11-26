# styxrpc_named_peer_read_ref_scope

[Русский](named_peer_read_ref_scope.ru.md) · [API](../include/styxrpc/named_peer.h#L50) · [Source](../src/named_peer.c#L303) · [Guide](module_refs.md)

`StyxRpc::serve`

```c
int styxrpc_named_peer_read_ref_scope(struct StyxClient *client, void *scope);
```

## Contract

Pass the remote named-service mount as `client`. Use `STYXRPC_MODULE_REF_ATTACH` when making that mount. The caller supplies a writable 16-byte `scope` buffer. The function walks the private file, verifies its advertised size and reads its complete payload. It closes the temporary fid on every outcome.

Success copies the scope and returns 1. Missing support, invalid arguments, incomplete transfer or incompatible file metadata return 0 and leave the output unchanged. The call blocks within the client's Styx deadlines. Keep the mount alive until completion. Pass the scope to [the descriptor codec](module_ref_decode.md).
