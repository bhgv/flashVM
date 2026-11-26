# vfs.newnode

[Русский](STYX_RPC_NEWNODE.ru.md) · [Default session](STYX_RPC_DEFAULT.md) · [Implementation](../src/styxrpc/session_nodes.cpp#L158)

`vfs.newnode(path, permissions, callback)` publishes a callback file in
the active movie's VFS. The path is rooted at the paired session,
independent of cwd. Missing parent directories are created with the file.
Permissions are distinct letters from `r`, `w` and `x`. Conflicting
paths, empty components, `.`, `..` and service names are refused.
Success returns `true`.

The movie owner calls `callback(op, buf, len)`. For `op == "r"` fill the
prefix of the byte Array and return the number produced. For `op == "w"`
the Array holds input bytes and the return value confirms the accepted
prefix. For `op == "x"`, the Array starts with the caller's bytes. Replace
its leading bytes with the result and return the length of that prefix.
The `x` permission works independently of `r` and `w`. Each invocation
waits for the complete request before calling the function once. Invoke it
from [ActionScript](STYX_VFS_EXEC.md), [Limbo](../../disVM/runtime/STYX-EXEC.md) or [C](../../lib/styxrpc/reference/node_exec.md)
over the existing session.

`len` is the input length and buffer capacity in bytes. Zero on a read is
EOF. Zero for an execution confirms an empty result. An exception or a
count outside 0–len fails the request. Ordinary reads and writes invoke
the callback once per transport operation and track position on the fid.

This AS2 example publishes a file that returns byte 42 in one read. The
peer accesses it using ordinary filesystem operations.

```actionscript
function readValue(op, buf, len) {
    if (len == 0) return 0;
    buf[0] = 42;
    return 1;
}
vfs.newnode("/app/value", "r", readValue);
```

The movie retains its callback until retirement or session shutdown. Open
fids retire with their nodes. Callback execution is synchronous with the
Styx request. The [native Styx test](../tests/styxrpc/node-files.c) verifies
read, write and execute buffers, mode checks and retirement of open fids.

[SDK callback node registration](../../lib/styxrpc/reference/module_slot_new_node.md).
