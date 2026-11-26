# Open VFS directory cursors

[Русский](vfs_directory.ru.md) · [Header](../include/styxrpc/vfs_client.h) · [Implementation](../src/vfs_directory.c)

`styxrpc_vfs_dir_open(session, path, &dir)` opens a visible directory at the
relative path, with an empty path for the attach root. On success the caller
owns the returned cursor and its fid. Each call to
`styxrpc_vfs_dir_next(dir, visit, state, &found)` advances one visible entry,
invoking `visit` with a borrowed name and metadata, or skips that entry when
the visitor is null. EOF sets `found` to zero with `STYXRPC_IO_OK`.
A malformed record or failed read stops the current iteration.
`styxrpc_vfs_dir_close(dir, deferred)` consumes the cursor. Pass 1 to defer
clunk to the attached session or 0 for synchronous close. Open, next and
synchronous close may wait for Styx I/O. Use one cursor from one owner.

Use [styxrpc_vfs_dir_rewind](vfs_dir_rewind.md) to restart on the same fid.
