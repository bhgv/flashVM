# A mounted Styx tree as a FlashVM filesystem

[Русский](STYX_RPC_FILESYSTEM.ru.md) · [FlashVM and DisVM](STYX_RPC.md) · [C SDK](../../lib/styxrpc/reference/vfs_open.md) · [Live VFS variables](STYX_RPC_LIVE_VARIABLES.md)

The host selects an attached Styx client. Link `flashvm_styxrpc` and pass the client to [fvm_styxrpc_filesystem_mount](STYX_RPC_FILESYSTEM_MOUNT.md) at a path such as `/dis`. Install the [file operations](FILESYSTEM_INSTALL.md) as `fs` in the same context. Script uses the same operations for local files and Styx files.

For paired operation mount at `/` in a fresh context. A root mount is exclusive. Every script path and ordinary file channel uses that session until context teardown. The ordinary view filters `.prog` from listings and rejects the component at any depth. Typed external variable bindings still use an authorized cell path.

```actionscript
var rows = fs.list("/dis");
var info = fs.stat("/dis/data/sample");
var input = fs.open("/dis/data/sample", "r");
var bytes = input.read(64);
input.close();
```

`stat(path)` returns `[lengthHigh, lengthLow, mode, directory]`. Two unsigned 32-bit length limbs preserve the exact 64-bit size. `list(path[, skip, limit])` returns up to 32 child entries. Each starts with the name, followed by the stat fields. To page, supply both the number of entries to skip and a limit of 1–32. The filesystem provides the ordering, and each page opens the directory independently.

`open(path[, mode])` defaults to read-only. A Styx tree supports `r`, `wo` to write an existing file, `rwo` with a shared cursor and `rw` to read from the start and append writes. Append measures the end of the open file before writing. Creation, truncation, rename and remove are outside the current Styx mount profile. Use `list` to open a directory.

A handle supports `read(count)`, `write(bytes)`, `seek(offset[, origin])` and `close()`. Input and output Arrays hold up to 4096 bytes including zero. EOF returns an empty Array. A successful write returns the number of bytes. Seek origins are the start (0), the current position (1) and the end (2). Append mode has separate cursors and refuses seek. A failed write can leave a confirmed prefix. When the complete write cannot be confirmed, the bridge stops writes on that handle.

`channel.create("file", path, mode)` and [fvm_channel_open_file_in](CHANNEL_OPEN_FILE_IN.md) resolve paths in the same context and retain the file channel protocol. A remote open holds a distinct fid until close. [Unmount](FILESYSTEM_UNMOUNT.md) changes subsequent path lookup. Open files keep their original fids. A path below a selected mount returns errors from that filesystem. Use nonempty components without `.` or `..` beneath the mount.

## External variables

[Bind a property](STYX_RPC_INT_VARIABLE_BIND.md) on an ActionScript object to the exact native `int` file under `.var`. Once bound, use ordinary expressions such as `trace(external.count)` and `external.count = 65539`. The owner holds the value, and the property reads or replaces it through `libstyxrpc`. Assigning to a read-only binding raises an exception. The garbage collector retains the binding and its fid through the property. [Native cell contract](../../disVM/runtime/VFS-INT-VARIABLE.md)

The [file operations test](../tests/styxrpc/vfs.c) connects to a standalone C device. The [variable test](../../disVM/native/terminal/tests/variable/guest.py) runs a native DisVM in interpreter and JIT modes. In each mode FlashVM observes the Limbo update and writes a new value while the read-only neighbor keeps its original value.
Run the native cross-VM check with the built DisVM runtime, bootstrap Limbo compiler, terminal SDK and FlashVM test host:

```sh
python3 disVM/native/terminal/tests/variable/guest.py \
  build-disvm/disvm build-disvm/bootstrap/limbo-bootstrap \
  build-terminal-sdk build-flashvm/flashvm_tests_styxrpc_host
```

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_channel_open_file_in` | function | [include/flashvm/channel.h:154](../include/flashvm/channel.h#L154) |
| `fvm_styxrpc_filesystem_mount` | function | [include/flashvm/styxrpc_filesystem.h:22](../include/flashvm/styxrpc_filesystem.h#L22) |
<!-- source-links:end -->
