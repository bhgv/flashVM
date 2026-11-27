# styxrpc_module_check_at

[Русский](module_check_at.ru.md) · [API](../include/styxrpc/module_client.h) · [styxrpc_module_check](module_check.md)

`styxrpc_module_check_at(directory, imports, count, pid)`

Check the same imports, count and optional caller PID as the path-based check, relative to an unopened fid obtained with styxrpc_module_open.

Validate signatures, schemas and accessible method files on the captured image.

STYXRPC_OK confirms all requested imports. STYXRPC_INVALID includes an invalid or withdrawn fid, inaccessible interface, mismatched requirements or an absent required PID. Inputs stay borrowed and unchanged. The caller serializes release or opening of directory and may wait for Styx I/O.
