# styxrpc.loadModule

[Русский](STYX_RPC_LOAD_MODULE.ru.md) · [Session](STYX_RPC_SESSION.md) · [Implementation](../src/styxrpc/session_modules.c)

```actionscript
var peer = styxrpc.loadModule("/mounted/peer.dis", ["magnitude"],
    ["realMagnitude", "managed", 0xbb977cfa, "(fs)", "(f)"]);
var value = peer.realMagnitude(-12.5, "sample");
```

Pass a path relative to the session's root, optionally with a leading slash,
and one to eight method descriptors. A one-field descriptor selects an Int32
function with the compiled `fn(int): int` signature. A five-field descriptor
contains an export name, `"managed"` or `"graph"`, an exact nonzero UInt32
signature, and the wire schemas of the argument and result lists.

The loader verifies the entire requested contract against the current image
and returns an object whose named methods invoke the remote functions.
An inaccessible image or contract mismatch raises a load error. The object
borrows the mounted session through the context lifetime. Each invocation
runs on the movie owner, rechecks the remote function and can wait for I/O.
The reply polling budget is 1000 ms after submission. Scoped imports require
a host-provided reverse PID. The stock player currently handles scalar and
unscoped shaped imports. See [publishObject](STYX_RPC_PUBLISH_OBJECT.md).
