# Vfsexport.bigvector

[Русский](VFS-BIG-VECTOR.ru.md) · [Module](../module/vfsexport.m) · [Implementation](vfs/cap_bigtable.c)

```limbo
bigvector: fn(): ref Root;
```

Load `Vfsexport` to create an empty growable array of signed Limbo `big`
values. Each call returns a separate VFS root or `nil` on allocation failure.
Mount the root or use `overlay` to publish it under `.var`. Both local
opens and reverse Styx opens of that root operate on the same storage.
The root and open files keep the storage alive.

| File | Access and data |
| --- | --- |
| `.type` | Read-only text `vector of big\n`. |
| `.len` | Read-only decimal length followed by a newline. |
| `.push` | Write one value to append it. |
| `.ins` | Write a position followed by a value. Insertion shifts later elements. The current length is a valid append position. |
| `.pop` | The first nonempty read removes and returns the last value. An empty array gives a read error. Short reads continue the same result. |
| `.idx` | Write one existing position. Keep the selector open while using `.val`. |
| `.val` | Read or overwrite the selected position. Write from offset zero. |

Each position and value is a signed eight-byte big-endian word. Positions
start at zero. Insertion accepts positions from zero through the current length.
For selection, negative positions count back from the end. The selector resolves
the position at write time. If an insertion shifts elements afterward, the open
selector still addresses that position. There is one selector per local process
or reverse call grant. Closing the selector revokes its open value files.
Open a fresh command FD for each insertion, append, or pop.

The [scoped Styx example](../examples/remote-scalar/styx-scoped-calls.b)
starts with an empty array. The [provider](../examples/remote-scalar/scoped-calls-owner.b)
appends 10, inserts 20, changes the last value to 21 and pops it. The caller
then reads length 1 in its own namespace. See also [fixed live cells](VFS-INT-ARRAY.md)
and [hybrid tables](VFS-HYBRID-TABLE.md).
