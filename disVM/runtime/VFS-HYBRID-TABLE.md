# Vfsexport.table

[Русский](VFS-HYBRID-TABLE.ru.md) · [Interface](../module/vfsexport.m) · [Implementation](vfs/cap_bigtable.c)

```limbo
table: fn(source: ref Root): ref Root;
```

Load `Vfsexport` and pass a live handle created by `Bigtable`. The
returned directory retains the same native storage. Invalid handles and
allocation failure return nil. Mount it or overlay it under `.var`. The
table still has one dense sequence, sparse numeric keys and text keys.

| File | Access and payload |
| --- | --- |
| `.type` | Read-only text `table of big\n`. |
| `.len` | Read-only decimal dense length followed by a newline. |
| `.count` | Read-only decimal number of all pairs followed by a newline. |
| `.ins` | Write signed 64-bit position and value, both big-endian. |
| `.push` | Write one signed big-endian 64-bit value. |
| `.pop` | Read the last dense value as a signed big-endian 64-bit value. An empty dense part refuses the read. |
| `.add` | Write tag byte 0 plus an eight-byte signed numeric key, or tag byte 1 plus 0–255 text bytes. Append one eight-byte signed value. All words are big-endian. |

Each mutation uses a fresh FD. A successful write consumes its operation.
The first nonempty read of `.pop` removes the entry. Short reads return
the rest of that same cached value. Insertion and pop preserve sparse and
text keys. Adding an existing key fails. A held native guard can defer a
VFS operation. The [scoped Styx example](../examples/remote-scalar/styx-scoped-calls.b)
checks remote positional and hash changes against local Bigtable reads.
