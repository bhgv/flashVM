# Vfsexport.record

[Русский](VFS-VARIABLE-RECORD.ru.md) · [Interface](../module/vfsexport.m) · [Implementation](vfs/cap_record.c)

```limbo
record: fn(fields: list of (string, ref Root)): ref Root;
```

Load `Vfsexport` from `Vfsexport->PATH`. Pass 0–254 distinct named
capabilities. Each name is a single path component and must begin
with a character other than a dot. The result is a directory with
one entry per field, a read-only `.type` containing `record\n`, and a
read-only `.len` containing the decimal field count and newline.
Invalid names, duplicate names, invalid roots or allocation failure
return nil.

A field retains its original capability and its access rules. It may
be a live [integer cell](VFS-INT-VARIABLE.md), a fixed
[array](VFS-INT-ARRAY.md), or a nested record. Multiple fields can
refer to the same root. The record membership is fixed at creation.
Publish the directory beneath `.var` to make it available to a scoped
remote call. The [Styx example](../examples/remote-scalar/styx-scoped-calls.b)
shows a record containing another record and an aliased array.
