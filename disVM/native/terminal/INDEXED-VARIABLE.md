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
selection. The fixture also exposes read-only `.len` and `.type`.

Run `ctest -R '^terminal-indexed-variable$' -V`. The fixture reads and
updates a selected element over Styx, rejects a competing selection and
invalidates the old value fid. Native checks also cover two connections,
out-of-range indices and partial writes.
