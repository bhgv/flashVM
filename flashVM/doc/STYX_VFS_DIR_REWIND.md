# Directory.rewind

[Русский](STYX_VFS_DIR_REWIND.ru.md) · [File and directory methods](STYX_RPC_OS.md) · [C SDK](../../lib/styxrpc/reference/vfs_dir_rewind.md) · [Implementation](../src/styxrpc/os.c#L385)

```actionscript
directory = vfs.opendir(path)
directory.rewind()
entry = directory.next_file()
```

Restart enumeration of the directory already open through `vfs.opendir`
or `os.opendir`. The method takes no arguments. It discards buffered
entries, clears EOF or a previous iteration failure, and returns `true`.
The next `next_file()` starts a new Styx read at offset zero. This step
can wait for normal I/O. If the server no longer recognizes the old
directory identity, reading fails. Call `opendir(path)` again if you
intend to resolve the pathname and open the directory that now exists
there. Calling `rewind()` on a closed handle raises an error.

The hosted callback tree has an independent revision for each directory.
Publishing or withdrawing an immediate child changes its revision. If the
server detects that an iteration spans two revisions, that read fails.
Changes in another directory let the iteration continue. An iterator may
still deliver records buffered by a previous read before it detects a
change. Start a fresh iteration when the caller needs current contents.

Limbo can restart an open Styx directory in the paired mount by calling
`sys->seek(fd, big 0, Sys->SEEKSTART)` and then `sys->dirread(fd)`. This
directory seek accepts only zero from the start. It also retains the
original fid.

See [live bind and unmount](../../disVM/runtime/STYX-LIVE-REBIND.md) for DisVM namespace changes.

To run the [paired check](../../disVM/runtime/STYX-CHANNEL.md#acceptance),
add `--dirlive`. The [AS movie](../../lib/styxrpc/tests/flashvm/dirlive_pair.as)
keeps one DisVM directory across a publication and a withdrawal. The
[Limbo caller](../../lib/styxrpc/tests/flashvm/dirlive_pair.b) reads a
multi-frame directory, verifies that a sibling directory's change does
not interrupt it, and rewinds after a change to the directory it is
reading. Both programs use the regular paired executables.
