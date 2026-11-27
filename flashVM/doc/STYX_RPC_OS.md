# File methods on vfs and os

[Русский](STYX_RPC_OS.ru.md) · [Player](STYX_RPC_PLAYER.md) · [Open and attributes](../src/styxrpc/os_open.c) · [Handle methods](../src/styxrpc/os.c)

In the stock player, `os` refers to the same object as `vfs`. Both names share the attached libstyxrpc session and its working directory.
A leading slash starts at the session root. Relative paths start at
[`pwd()`](STYX_VFS_PWD.md). The initial working directory is `"/"`. Service paths including `.prog`, `.var` and UniBus roots
are hidden.

[Publish a callback-backed file from Limbo](../../disVM/runtime/STYX-NODES.md). It uses these same file methods.

| Call | Result |
| --- | --- |
| `vfs.open(path[, mode])` | Open a file. Modes are `"r"` by default, `"w"` for write without truncation on open, `"rw"` for both directions, and `"wx"` to create a new write-only file. |
| `vfs.opendir(path)` | One open directory fid with a forward iterator. |
| `vfs.attr(path[, root])` | Live entry attributes. If supplied, `"/"` selects the mounted VFS. |
| [`vfs.mkdir(path)`](STYX_VFS_MKDIR.md) | Create one directory. |
| [`vfs.rm(path)`](STYX_VFS_RM.md) | Remove a file or empty directory. |
| [`vfs.rmnode(path)`](STYX_RPC_RMNODE.md) | Withdraw one callback file published by the movie. |
| [`vfs.cd(path)`](STYX_VFS_CD.md) | Change the shared working directory. |
| [`vfs.pwd()`](STYX_VFS_PWD.md) | Read the canonical working path. |
| [`vfs.exec(path[, bytes])`](STYX_VFS_EXEC.md) | Call an executable node and return its byte reply. |

`"wx"` requires an existing directory that permits file creation. An occupied
name causes an error. In the stock pair, DisVM applies the current VFS
rules and the host file creation policy. Providers without creation support
return an error. Call on the movie owner thread. I/O can wait up to the
Styx request deadline.

Attributes and `next_file()` results expose `name`, `directory`, `mode`,
`lengthHigh` and `lengthLow`. The last two are the exact UInt32 limbs of the
byte length. `length` is an AS Number up to 2⁵³−1 or `undefined`.
`next_file()` returns `undefined` at EOF. Use [`rewind()`](STYX_VFS_DIR_REWIND.md)
to start another enumeration on the same open directory.

| File method | Result |
| --- | --- |
| `read(count)` | Array of confirmed bytes at the current offset. EOF can yield a short array or an empty one. |
| `write(bytes)` | Confirmed byte count. Pass a dense array of integers 0–255. |
| `seek(delta[, origin])` | New byte offset. The default `"set"` starts at zero. `"cur"` and `"end"` use the current position or live EOF. |
| `len()` | Current opened file length in bytes. |
| `flush()` | Write acknowledgement state. The client sends writes immediately. A false result means the last acknowledgement was lost. Persistence is determined by the provider. |
| `load()` | Child `MovieClip` for a `.swf` or a remote method proxy for a published `.dis` image. |
| `close()` | Close a file or iterator. A repeated close succeeds. |

Each read and write accepts up to 2048 bytes. Confirmed bytes advance the
cursor. Operation errors reach the script. An uncertain write requires a
fresh open before further writes. When the handle and extracted methods
become unreachable, collection queues the fid clunk outside GC.

```actionscript
var file = vfs.open("/movies/intro.swf");
var clip = file.load();
file.close();
clip.play();

var image = os.open("/mounted/counter.dis");
var counter = image.load();
image.close();
var result = counter.next(41);
```

A SWF is read in full from the held fid independently of its cursor. The
player attaches the clip synchronously at the root movie's next public depth.
The usual player loop runs its frames. The clip retains the source path for
subsequent relative loads.

A `.dis` load uses a published StyxRPC interface. It discovers names,
signatures and schemas for direct calls. The stock pair supports Int32 and
unscoped managed/graph profiles. Scoped calls require a reverse PID. Ordinary
`.dis` files in the VFS need an interface published by DisVM. A module
descriptor captures its exact publication when opened. `load()` checks that
publication and the proxy holds its own fid, so closing the source descriptor
leaves the loaded clip or proxy usable. If the owner withdraws an image, held
descriptors and proxy methods fail even when another image uses the same name.

A detached task in the stock pair receives a reverse PID and an argument bus.
It can load a scoped published `.dis`, lend a local queue to a method and use
its result channel. Scoped calls use a detached task while the film serves
the reverse requests. See [VFS in tasks](ASINC_VFS.md).

Acceptance sources are [AS](../tests/styxrpc/os-client.as) and
[Limbo](../../lib/styxrpc/tests/flashvm/os_check.b). For explicitly selected
contracts see [styxrpc.loadModule](STYX_RPC_LOAD_MODULE.md).
