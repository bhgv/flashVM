# Indexed Styx array file

[Русский](INDEXED-VARIABLE.ru.md) · [Header](indexed_variable.h) · [Example](tests/indexed_variable.c)

The `styx_terminal` library associates an index selection with a variable
context and Styx connection. Allocate one `StyxIntArray` per exported
process variable. The owner supplies its extent, storage and callbacks.
Serialize calls to the same object on a dispatcher or under an external lock.

1. Open `.idx` for writing. Only one selection is active at a time.
2. Write a four-byte, big-endian zero-based index at offset zero. The server
   verifies the extent and keeps that index until close.
3. With `.idx` still open, open `.val` on the same connection. Read or
   write the selected element in [scalar format](VARIABLE-FILE.md).
4. Close `.val` and `.idx`. Closing or disconnecting `.idx` invalidates
   any remaining `.val` fids from that selection.

Keep the returned generation in each opened fid. Call `styxintarrayclose`
only for the selector in `freefid`. A stale value fid cannot access a new
selection. The adapter also exposes read-only `.len` and `.type`.

Run `ctest -R '^terminal-indexed-variable$' -V`. The fixture reads and
updates a selected element over Styx, rejects a competing selection and
invalidates the old value fid. Native checks also cover two connections,
out-of-range indices and partial writes.

## Server integration

`styxarrayfilepublish` from `styx_unibus_server` creates the array directory
below an existing parent qid, such as a granted `.prog/<pid>/.var`.
Include `lib9.h`, `styx.h`, `styxserver.h` and `indexed_styx.h`.
Before running the dispatcher, supply the directory name, owner, array and
a range of consecutive unused qids. Overlapping qid ranges are rejected before
publication. Keep the array and publication descriptor alive through fid
retirement.

Route the range's operations to `styxarrayfileopen`, `styxarrayfileread`
and `styxarrayfilewrite`. Use `styxarrayfileclose` in `freefid` for these
opens. A server with several kinds of fid contexts must dispatch their
cleanup to the corresponding handler. With no `store` callback, `.val`
is read-only. The library publishes `.len` and `.type`.


## Changing the length

A fixed array uses five qids. An `insert` callback requires seven, and a
`pop` callback requires eight. The library checks the entire range before
publication.

| File | Operation |
| --- | --- |
| `.ins` | Write eight bytes at offset zero, four for the index and four for the value. An index equal to the length inserts at the end. |
| `.push` | Write one four-byte value at offset zero to append. |
| `.pop` | Read four bytes at offset zero. The first successful read removes the last element. Another read at offset zero on the same fid returns the cached value. |

All fields are big-endian and values are signed. Each successful mutation
changes `.len` by one. An open `.idx` prevents new structural mutations.
Popping an empty array returns a Styx error. Owner callbacks must be atomic:
return 1 after a committed change, or 0 with storage unchanged. The owner
keeps storage consistent with the adapter's length. The example exercises
all three operations over Styx.
