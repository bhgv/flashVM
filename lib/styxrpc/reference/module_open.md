# styxrpc_module_open

[Русский](module_open.ru.md) · [API](../include/styxrpc/module_client.h) · [styxrpc_module_check](module_check.md)

`styxrpc_module_open(session, path)`

Walk a visible relative module directory on the already attached session. The server must advertise a directory with read and search rights.

Resolve a directory once for subsequent interface inspection and method calls.

Return an owned unopened module fid on success or NULL on lookup, permission or metadata failure. The fid pins the connection. Release it with styxclientclunk, or use styxrpc_vfs_close to defer wire cleanup when required. Serialize open or release against all borrowers. The call can wait for Styx I/O.
