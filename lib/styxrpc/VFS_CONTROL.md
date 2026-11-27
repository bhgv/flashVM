# Cancellation during mounted VFS operations

[Русский](VFS_CONTROL.ru.md) · [SDK guide](README.md) · [Ordinary tree providers](reference/server_enable_mutations.md)

An ordinary path operation may walk several components, read metadata, then open,
create or remove an exact fid. Use the `_until` SDK entry points when the caller
can stop during those stages. They send native Styx requests over the existing
attachment and use an ordinary tag-specific `Tflush` for an outstanding wait.
A stop hook reads the caller's flag on the calling thread, including while a
multiplexed reply is pending.

| Operation | SDK entry point |
| --- | --- |
| Inspect a path or an existing file | [stat](reference/vfs_stat_until.md), [file stat](reference/vfs_file_stat_until.md) |
| Open a regular file or directory | [file open](reference/vfs_open_until.md), [directory open](reference/vfs_dir_open_until.md) |
| Step a directory cursor | [directory next](reference/vfs_dir_next_until.md) |
| Change the namespace once | [exclusive file create](reference/vfs_create_until.md), [mkdir](reference/vfs_mkdir_until.md), [remove](reference/vfs_remove_until.md) |
| Transfer data on a held file | [read](reference/vfs_file_read_until.md), [write](reference/vfs_file_write_until.md) |

An original acknowledgement can win the race with its flush. The SDK publishes
the confirmed result in that case. Cancellation of path admission leaves the
output unchanged and retires temporary fids. A cancelled directory step retires
its stream, so close the cursor and reopen the path before reading again. A file
stat borrows its open fid and does not consume it when its metadata wait stops.

A mutation stop before submission returns `STYXRPC_IO_CANCELLED`. A sent
`Tcreate` or `Tremove` without a reliable original reply returns
`STYXRPC_IO_UNCERTAIN`. The provider may already have applied the change. The SDK
never automatically retries the mutation. Use a separate lookup to check the
present state if it matters to the application.

On a serial transport, the caller can stop before each stage. Once an exchange
has entered that transport, it uses the configured deadline. A multiplexed
client still needs a peer that can process a flush or return the original reply.
Synchronous provider callbacks control how soon the peer can process new
requests.

## Try it

Run `ctest --test-dir build-styxrpc -R styxrpc-vfs-control-wire --output-on-failure`.
The [wire fixture](tests/vfs_control_wire.c) holds a selected original acknowledgement,
reads through another fid and flushes the targeted tag. Mutation cases exercise
both possible effects and acknowledgements that win the race. The wire test
also holds the second fragment of a long walk and checks that a cancelled
directory cursor cannot be rewound.

The main project [AS2 task guide](https://github.com/bhgv/flashVM/blob/master/flashVM/doc/ASINC_VFS.md)
runs a real FlashVM–DisVM pair using the same wire contract.
