# Inspect a resident call stack

Copyright (c) 2026 bhgv

[Русский](README.ru.md) · [Contract](../../runtime/OWNER-STACK.md)

The example creates one real guest with non-tail recursion, copies its stack
through the owner's independent diagnostic mailbox, joins the guest, then prints
the surviving caller-owned display data. Other native owners are not paused.

From the repository root, using a matching built NC64 compiler/runtime:

```sh
build/disvm-opt/bootstrap/limbo-bootstrap -I disVM/upstream/module -I disVM/module \
  -o build/disvm-opt/residentstack.dis disVM/debug/residentstack.b
build/disvm-opt/bootstrap/limbo-bootstrap -I disVM/upstream/module -I disVM/module \
  -o build/disvm-opt/resident-stack.dis disVM/examples/resident-stack/main.b
build/disvm-opt/disvm --stats --no-jit build/disvm-opt/resident-stack.dis
build/disvm-opt/disvm --stats --jit build/disvm-opt/resident-stack.dis
```

Keep residentstack.dis beside the caller image. Success prints five frame rows,
`PASS resident stack example`, then zero retained diagnostic credits at shutdown.
Exact PCs depend on the matching image and capture boundary. Top is the next Dis
PC; lower rows are saved caller return PCs. These are not source line numbers.

Call `Residentstack->inspect(id, 1000)` in your application. Check status, sequence
and StackComplete/StackTruncated/StackUnavailable. At most eight top-first frames
are copied. Strings are bounded UTF-8 display labels, not global module handles.
A Ready ticket may have no available frames; Pending means the owner has not
answered. No local variables, native addresses, pause or external attach is provided.
