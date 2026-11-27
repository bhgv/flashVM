# styxrpc_module_live_open

[Русский](module_live_open.ru.md) · [Declaration](../include/styxrpc/module_client.h) ·
[Implementation](../src/live_module_client.c)

```c
#include <styx/client/client.h>
#include <styxrpc/module_client.h>

struct StyxFid *styxrpc_module_live_open(struct StyxClient *session,
    const char *path, struct StyxFid **lease);
```

The hosted SDK asks the attached DisVM session to load a regular `.dis`
file. Borrow `session` and a canonical, visible, root-relative `path` of at
most `STYXRPC_VFS_PATH_MAX` bytes. The path is resolved by DisVM during the
load. Pass a writable slot for the lease. On success the return value is
an owned unopened directory and `*lease` is an owned control fid. On
failure the function returns NULL and leaves `*lease` unchanged.

The operation polls for publication for up to four seconds, subject to
the session's native I/O timeouts. It sends one request and never replays
it. A missing service, refused image, refused publication or transport
failure yields NULL. Use the call on a thread permitted to wait for
Styx I/O. Each invocation owns its request cursor.

```c
struct StyxFid *lease = NULL;
struct StyxFid *module = styxrpc_module_live_open(session, "lib/counter.dis", &lease);
StyxRpcModuleExports exports;
if (module && styxrpc_module_exports_at(module, 0, &exports) == STYXRPC_OK) {
    /* Use exact-fid calls with the verified exports here. */
}
styxclientclunk(module);
styxclientclunk(lease);
```

Keep the lease through every call and any returned live channel. Closing
it revokes the loaded generation. See [export discovery](module_exports_at.md)
and [exact Int32 calls](scalar_invoke_at.md).
