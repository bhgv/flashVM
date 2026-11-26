# styxrpc_named_peer_host_scoped

[Русский](named_peer_host_scoped.ru.md) · [API](../include/styxrpc/named_peer.h#L37) · [Source](../src/named_peer.c#L238) · [Guide](module_refs.md)

`StyxRpc::serve`

```c
StyxRpcNamedPeer *styxrpc_named_peer_host_scoped(struct RemotePhy *lane,
    StyxRpcNamedRegistry *registry, unsigned msize, unsigned timeout,
    unsigned tags, const void *scope);
```

## Contract

Use the [hosted peer constructor's](named_peer_host.md) `lane` and borrowed `registry` with `msize` in bytes, `timeout` in milliseconds and `tags` limiting pending local Styx requests. Pass 16 fresh random bytes in `scope` for this pairing or NULL to omit the module reference capability. The host copies the scope before starting the service threads.

The function consumes `lane` on every outcome. It can block while mounting the local client. It returns the owned peer or NULL on failure. Keep the registry alive through [destruction](named_peer_destroy.md). The remote side can [read the scope](named_peer_read_ref_scope.md) after attaching to the named lane.
