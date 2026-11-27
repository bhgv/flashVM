# Vfsnative.scalarloadedexports

[Русский](VFS-SCALAR-LOADED-EXPORTS.ru.md) ·
[Limbo declaration](../module/vfsnative.m) ·
[Native owner](vfs/loaded_owner.c)

```limbo
scalarloadedexports: fn(anchor: Scalarloadedanchor): string;
```

Load the `Vfsnative` builtin in a running DisVM process. Pass the non-nil
anchor produced by `scalarloadanchor(path)`. The builtin copies callable
export names in compiled order, separated by tabs. It uses the owner
gateway's signature, shape and frame checks. It skips exports with no
supported call profile. A nil anchor, an empty catalogue or more than
eight callable entries returns nil. The limit matches one mounted module's
export table and prevents a partial publication from masquerading as the
complete catalogue.

Discovery reads the loaded module instance. It performs no file I/O,
runs no entry point and keeps the anchor alive only through the call.
The returned string owns its data. Keep the anchor for later binding.
After a path replacement, discover and bind on the old anchor to keep
using that exact image. A fresh `scalarloadanchor(path)` resolves the
current file again.

Pass `nil` as the `offer` argument to `scalarloadedview` or
`scalarloadedslot` to bind privately. They return the normal typed
view or gateway of the same instance without writing the wire offer.
With a non-nil offer path, the existing publication behavior applies.

The [Limbo test](tests/loaded_exports/client.b) discovers scalar, managed
and channel exports while skipping an unsupported list shape. It holds
an anchor across a file replacement and verifies that independently linked
scalar views share its original data. The
[runner](test_loaded_exports.py) compiles the images with the project
bootstrap compiler and runs the test under either native execution mode.

Use [scalarloadedprofile](VFS-SCALAR-LOADED-PROFILE.md) to copy the corresponding private wire offers from the same anchor.
