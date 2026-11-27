# styxrpc_module_exports_at

[Русский](module_exports_at.ru.md) · [API](../include/styxrpc/module_client.h) · [styxrpc_module_exports](module_exports.md)

`styxrpc_module_exports_at(directory, pid, out)`

Read the compiled contracts and discover callable methods relative to the borrowed unopened directory. PID has the path-based discovery meaning.

Discover exports of an image already selected by an earlier open.

STYXRPC_OK publishes the checked exports in out. Keep out at its original address because method names point inside it. STYXRPC_INVALID leaves it unchanged. The caller serializes open and release of directory. The operation can block on Styx.
