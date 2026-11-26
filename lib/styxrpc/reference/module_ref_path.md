# styxrpc_module_ref_path

[Русский](module_ref_path.ru.md) · [API](../include/styxrpc/module_ref.h#L33) · [Source](../protocol/module_ref.c#L28) · [Guide](module_refs.md)

`StyxRpc::rpc`

```c
int styxrpc_module_ref_path(const char *path);
```

## Contract

Pass a NUL-terminated path. The call returns 1 for the literal form `mounted/<image>/.var/<variable>`. Each name has 1 to 255 bytes, excludes slashes and must differ from `.` and `..`. A leading slash, empty name or NULL yields 0. The function borrows the string for the call, performs no VFS requests and can run on any thread.

The path `mounted/counters.dis/.var/count` passes. Continue with [identified open](vfs_int_open_identified.md) to verify the actual node.
