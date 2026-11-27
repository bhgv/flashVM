# Vfsnative.scalarloadedprofile

[Русский](VFS-SCALAR-LOADED-PROFILE.ru.md) · [Catalogue names](VFS-SCALAR-LOADED-EXPORTS.md) ·
[Limbo declaration](../module/vfsnative.m) · [Implementation](vfs/loaded_catalogue.inc)

```limbo
scalarloadedprofile: fn(anchor: Scalarloadedanchor): array of byte;
```

Load the `Vfsnative` builtin in a running DisVM process and obtain an
`anchor` with `scalarloadanchor(path)`. For example:

```limbo
anchor := routes->scalarloadanchor("library.dis");
profile := routes->scalarloadedprofile(anchor);
```

A successful call returns a new byte array containing one private S-record
offer per callable export, in the same compiled order as
`scalarloadedexports(anchor)`. Each offer carries the compiled name and
signature and, when present, the argument and result shapes. Admission uses
the same gateway checks as `scalarloadedslot`. A nil anchor, an empty
catalogue, more than eight callable exports, or an encoding failure yields
nil. No partial profile is returned.

The call reads only the loaded instance. It performs no file I/O and invokes
no guest methods. The caller owns the array, and changing it has no effect
on later calls. Retain the anchor to bind views of the same module instance
after a path replacement.

A Styx owner can use these records when preparing a publication. The private
offers describe the compiled methods. Session scope, dispatch routes and
the public `.interface` belong to the publication step.

The native [publication validator](../native/terminal/publish_profile.h)
checks the full sequence and stages the public profile. It rejects malformed
records, duplicate names and reserved children. When the session supplies
a reverse root, shape-driven calls receive the corresponding scoped tags.
