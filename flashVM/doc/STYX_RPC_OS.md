# The os system object

[Русский](STYX_RPC_OS.ru.md) · [Player](STYX_RPC_PLAYER.md) · [Open and attributes](../src/styxrpc/os_open.c) · [Handle methods](../src/styxrpc/os.c)

`os` is installed in the stock player when a libstyxrpc VFS is mounted. Paths
with and without a leading slash start at that session root. `"/"` names the
root directory. Service paths including `.prog`, `.var` and UniBus roots
are hidden.

| Call | Result |
| --- | --- |
| `os.open(path[, mode])` | Open an existing file. Modes are `"r"` by default, `"w"` for write without truncation on open, and `"rw"` for both directions. |
| `os.opendir(path)` | One open directory fid with a forward iterator. |
| `os.attr(path[, vfs])` | Live entry attributes. If supplied, `"/"` selects the mounted VFS. |

Attributes and `next_file()` results expose `name`, `directory`, `mode`,
`lengthHigh` and `lengthLow`. The last two are the exact UInt32 limbs of the
byte length. `length` is an AS Number up to 2⁵³−1 or `undefined`.
`next_file()` returns `undefined` at EOF. Reopen for a fresh enumeration.
Changes to a directory can interrupt an open enumeration.

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
var file = os.open("/movies/intro.swf");
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
`.dis` files in the VFS need an interface published by DisVM. Closing the
handle leaves the loaded clip or proxy usable.

Acceptance sources are [AS](../tests/styxrpc/os-client.as) and
[Limbo](../../lib/styxrpc/tests/flashvm/os_check.b). For explicitly selected
contracts see [styxrpc.loadModule](STYX_RPC_LOAD_MODULE.md).
