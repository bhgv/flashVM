# rpc.newnode

[Русский](STYX_RPC_NEWNODE.ru.md) · [Default session](STYX_RPC_DEFAULT.md) · [Implementation](../src/styxrpc/session_nodes.cpp)

`rpc.newnode(path, permissions, callback)` publishes a callback file in
the active movie's VFS. The path is rooted at the paired session,
independent of cwd. Missing parent directories are created with the file.
Permissions are distinct letters from `r`, `w` and `x`. Conflicting
paths, empty components, `.`, `..` and service names are refused.
Success returns `true`.

The movie owner calls `callback(op, buf, len)`. For `op == "r"` fill the
prefix of the byte Array and return the number produced. For `op == "w"`
the Array holds input bytes and the return value confirms the accepted
prefix. `len` is the number of bytes. Zero on a read is EOF. An exception
or count outside 0–len fails the request. The `x` bit is recorded in
metadata. The wire format for passing arguments to an execute request is pending.
The callback runs for each transport request. Its three arguments carry the request mode, copied bytes and count. Position belongs to the underlying fid.

This AS2 example publishes a file that returns byte 42 in one read. The
peer accesses it using ordinary filesystem operations.

```actionscript
function readValue(op, buf, len) {
    if (len == 0) return 0;
    buf[0] = 42;
    return 1;
}
rpc.newnode("/app/value", "r", readValue);
```

The movie retains its callback until retirement or session shutdown. Open
fids retire with their nodes. Callback execution is synchronous with the
Styx request. The [native Styx test](../tests/styxrpc/node-files.c) verifies
read and write buffers, mode checks and retirement of an already open fid.

[SDK callback node registration](../../lib/styxrpc/reference/module_slot_new_node.md).
