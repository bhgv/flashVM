# Real filesystem metadata and directory traversal

[Русский](README.ru.md) · [Contract](../../runtime/FILESYSTEM.md) · [Source](main.b)

This executable Limbo tutorial checks the hosted Sys filesystem boundary used by
an IDE file browser. It calls the actual Sys builtin and original Readdir library;
it does not mock native metadata or directory batches.

Requirements: a matched native Limbo compiler, DisVM, `sys.m`, `readdir.m`, and
compiled `readdir.dis`. The checked-in source has a deliberate `FIXTURE_ROOT`
placeholder. The [driver](../../runtime/test_filesystem.py) creates a private
fixture, substitutes its quoted UTF-8 path, compiles, runs, checks output and
removes its own files. Do not compile the template without substitution.

From the repository root after building the runtime:

```sh
python3 disVM/runtime/test_filesystem.py build/runtime/bootstrap/limbo-bootstrap \
  build/runtime/disvm tools/limbo-bootstrap/upstream/module \
  disVM/examples/filesystem/main.b build/runtime/readdir.dis jit
```

Use the actual compiler location in your build (`find build/runtime -name
limbo-bootstrap` if needed). Repeat with `interpreter`. The registered CTest tests
supply the exact compiler path automatically:

```sh
ctest --test-dir build/runtime -R 'sys_filesystem|shared_filesystem' --output-on-failure
```

In an installed runtime SDK, from its directory:

```sh
python3 examples/filesystem/run.py ./limbo-native ./disvm ./include/limbo \
  ./examples/filesystem/main.b ./readdir.dis jit
```

The expected final driver output is `PASS hosted Sys filesystem jit`. The driver
requires the Limbo PASS marker in both quantum-1 and quantum-2048 runs. It creates
73 entries, including a Unicode binary file and a sparse 4294967303-byte file;
filesystem sparse-file support and available metadata quota are required.

The client verifies exact lengths and identity, binary NUL, preserved file cursor,
three directory batches, EOF, Readdir name sorting and repeated FD retirement.
Missing paths and embedded NUL clear old result owners. `content://` is rejected.
Every source line has a tutorial comment; function comments describe the checks.

| Sys/library API | Purpose and ownership |
| --- | --- |
| `stat`, `fstat` | Copy metadata to VM-owned records; fstat preserves positions. |
| `open`, `read`, `seek` | Own an FD, verify actual bytes and reject directory byte streams. |
| `dirread` | Allocate VM-owned arrays in bounded batches; nil means EOF/error as indicated by count. |
| `Readdir.init` | Enumerate/sort through Sys; returned records survive descriptor closure. |

Dropping the last FD reference closes its native resource. Assigning nil to arrays
releases their owners. The driver alone owns and cleans temporary fixture files.
The native API and all failure/blocking contracts are described in the linked
[guide](../../runtime/FILESYSTEM.md#portable-api-ownership) and
[header](../../../flashVM/ports/filesystem_port.h).

Linux is the executed native acceptance target. Windows/macOS/Android selection
is implemented but still needs native testing. This example does not implement
background I/O, a namespace, Android SAF permissions or the debugger `/prog` service.
