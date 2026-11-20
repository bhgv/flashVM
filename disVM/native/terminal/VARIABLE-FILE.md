# Scalar Styx proxy

[Русский](VARIABLE-FILE.ru.md) · [Header](variable_file.h) · [Example](tests/link/images.c)

The `styx_terminal` library provides `styxintvariableread` and
`styxintvariablewrite` for Styx file callbacks. A `StyxIntVariable`
holds the borrowed owner context and its `load` and `store` callbacks.
Each callback returns 1 on success. The owner provides context lifetime
and synchronization with other threads.

Each value occupies four bytes in signed big-endian format. A read at offset
zero returns the whole current value from the owner. Offsets at or beyond
four return EOF. A write accepts exactly four bytes at offset zero and sends
the decoded value to the owner once. Partial writes are rejected before
`store`. The Styx server supplies file permissions.

Run `ctest -R '^terminal-ordered-files-guest$' -V`. The Dis parent writes a
negative value through `/.prog/.self/.var/counter`. Its child reads that
value and writes a positive value spanning multiple bytes. The parent reads
the update. See the [process context route](../../runtime/LAUNCH-PROGRAM-ROUTE.md).

For array elements, see [indexed variable files](INDEXED-VARIABLE.md).
