# Inspect a running resident

Copyright (c) 2026 bhgv

[Русский](README.ru.md) · [Runtime contract](../../runtime/OWNER-LOCATION.md)

The example starts a real guest on its permanent OS thread, obtains its exact
resident identity, asks the owner for a copied location, and joins the guest.
It prints module/path, next Dis PC, instruction extent and validity/truncation
flags. It runs inside the observed VM; it does not attach to another OS process.

From the repository root, using an already built matching NC64 runtime/compiler:

```sh
build/disvm-opt/bootstrap/limbo-bootstrap -I disVM/upstream/module -I disVM/module \
  -o build/disvm-opt/residentdebug.dis disVM/debug/residentdebug.b
build/disvm-opt/bootstrap/limbo-bootstrap -I disVM/upstream/module -I disVM/module \
  -o build/disvm-opt/resident-debug.dis disVM/examples/resident-debug/main.b
build/disvm-opt/disvm --stats --no-jit build/disvm-opt/resident-debug.dis
build/disvm-opt/disvm --stats --jit build/disvm-opt/resident-debug.dis
```

Keep the reader beside the caller image. The runtime's sibling-module resolver
finds residentdebug.dis there. Compile both against the same Processctl interface.
A successful run ends with `PASS resident debug example` and zero retained native
diagnostic credits. The exact next PC may vary with instruction quantum.

Use `Residentdebug->inspect(id, 1000)` from your own application. Check status and
LocationValid before interpreting PC. Path/name are display labels, not global
Module handles. Truncated strings are complete UTF-8 prefixes. This example does
not walk stack frames, inspect variables or pause another guest.
