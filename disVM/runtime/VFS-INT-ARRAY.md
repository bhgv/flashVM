# Vfsexport.intarray

[Русский](VFS-INT-ARRAY.ru.md) · [Module](../module/vfsexport.m) · [Implementation](vfs/cap_array.c)

```limbo
intarray: fn(cells: list of ref Root): ref Root;
```

Load `Vfsexport` from `Vfsexport->PATH`. Pass 0–252 native integer
roots created by [intvar](VFS-INT-VARIABLE.md). The result is a directory
with zero-based decimal filenames, a read-only `.len` containing a decimal
count and newline, and a read-only `.type` containing `array of int\n`.
Element I/O uses the signed, four-byte big-endian integer format.

The directory retains the supplied cells. Reusing a root in two positions
makes both names refer to that cell. Invalid roots, excess entries, or
allocation failure return nil. Mount the returned root or overlay it
beneath a published `.var`. Directory membership is fixed at creation.
The [scoped Styx example](../examples/remote-scalar/styx-scoped-calls.b)
exports three positions backed by two cells and checks a remote write
through the ordinary caller VFS.

### Select an element through an open FD

Open `.idx` for writing and send one four-byte unsigned big-endian index.
While that FD remains open, open `.val` for reading or writing. Its cursor
belongs to the selected cell. Closing `.idx` revokes that selection and
its opened `.val` fids. Another local process or reverse Styx grant can
hold an independent selection.

Arrays can be fields of a [VFS record](VFS-VARIABLE-RECORD.md).
