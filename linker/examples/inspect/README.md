# Inspect from a C application

This executable tutorial validates a SWO1 object or SWA1 library and reports its
modules without a subprocess or SWF generation. Suitable for an IDE's input
browser. It uses only SWL's public C99 interface, not FIR or compiler internals.
[Source](main.c), [API and lifetime contract](../../INSPECTION.md).

```sh
cmake -S linker/examples/inspect -B build/inspect -DSWL_ROOT="$PWD/linker"
cmake --build build/inspect
build/inspect/inspect_example library.swa
```

For the cross SDK pass the same toolchain/system/CPU/compiler arguments as in
[the build guide](../../../BUILDING.md); SWL_ROOT must identify a host-readable
source directory. Run the resulting program only on its target OS/CPU. Renaming
or detaching the example and SDK is supported through the explicit SWL_ROOT.

| API | Role and ownership |
| --- | --- |
| `swl_inspect` | Synchronous full validation and visitation. Borrows immutable input until return; returns 0 on format, memory or callback failure. No console/file access. |
| `SwlModuleVisitor` | Host callback; returns 0 to cancel. Metadata/name are borrowed only for its duration. The sample retains a module count, never borrowed pointers. |
| `SwlModuleInfo` | Name, code byte count and extension requirements; reports requirements without enabling execution. |

Expected output for a one-module object named alpha with only ActionEnd:

```text
alpha: 1 code bytes; requirements=0
modules: 1
```

Each nonempty source line explains input ownership, reporting or cleanup.
The host reads seekable regular files of 1..64 MiB, closes the stream, validates,
then frees the buffer. It detects changed length but does not guarantee an atomic
snapshot against concurrent same-length writes. Input files must be stable.
Exit 0 means complete reporting; 1 read/format/allocation/output failure; 2 usage.
Prevalidation failure produces no module rows. A callback/output failure may
leave partial console text. This does not verify AVM1 stack safety or termination.
