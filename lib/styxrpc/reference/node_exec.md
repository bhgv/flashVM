# styxrpc_node_exec

[Русский](node_exec.ru.md) · [Declaration](../include/styxrpc/nodes.h#L57) · [Client](../src/node_exec_client.c#L47) · [Provider](../src/node_exec.c#L81)

```c
#include <styxrpc/nodes.h>
StyxRpcIoOutcome styxrpc_node_exec(struct StyxClient *session, const char *path,
    void *bytes, size_t size, size_t *used);
```

Link `StyxRpc::launch`. The peer publishes an executable callback via
[`styxrpc_module_slot_new_node`](module_slot_new_node.md) on its hosted slot
server. Each call opens one private fid on the existing session. It blocks
until the callback returns or the operation fails.

| Parameter | Meaning |
| --- | --- |
| `session` | Borrowed attached client. Keep it live through the call. |
| `path` | Nonempty relative path from the attach root. At most 1024 bytes. Service names, empty components, `.` and `..` are refused. |
| `bytes` | Caller-owned writable buffer with caller input. On success its leading `*used` bytes become the reply. NULL is allowed when `size` is zero. |
| `size` | Input byte count and capacity for the reply, from 0 to `STYXRPC_NODE_BUFFER_MAX` inclusive. |
| `used` | Required output. Zero on failure and the reply length on success. |

`STYXRPC_IO_OK` reports a confirmed result. An empty result is valid.
`STYXRPC_IO_NOT_OPEN` reports invalid arguments or inability to open the
service. Other outcomes report failure or uncertainty. A lost acknowledgement
or a reply failure can occur after the callback has run. The client releases
the fid at return and never replays the call. On failure it leaves `bytes`
untouched. Concurrent calls use separate fids. The provider invokes callbacks
on its owner thread, and closing an incomplete write discards it.

```c
unsigned char bytes[] = {38};
size_t used;
StyxRpcIoOutcome status = styxrpc_node_exec(session, "menu/invoke",
    bytes, sizeof(bytes), &used);
```

On the [paired native fixture](https://github.com/bhgv/flashVM/blob/master/flashVM/tests/styxrpc/node-files.c)
this invocation returns one byte with value 42. Check `status` before using
the reply.

## Wire exchange

Walk `/.exec` from the attach root. Its stat identifies a regular service
node with `STYXRPC_NODE_EXEC_QID` (3). Open it for reading and writing.
The directory enumerator omits this service. The request starts at offset
zero with two unsigned 32-bit big-endian counts for the path field and
argument. The path field includes one terminating zero. It precedes the
argument bytes. The count for the argument may be zero. The final write
executes the node and confirms the request on success. Read the stored
reply starting at offset zero. Its length is at most the argument count.
Clunk the fid after the result. Each fid admits one request.
