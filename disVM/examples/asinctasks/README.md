# Cooperative task ownership example

[Full protocol, ownership and build guide](../../../compiler/runtime/ASINC-TASKS.md)

`main.b` is a portable Limbo bytecode application, built as `tasks-demo.dis`.
It loads `asinctasks.dis`, submits an immutable string, spawns a separate Limbo
worker and waits on an explicit channel. The worker claims input with its own
identity and lease, completes it, and the consumer takes the result once before
retiring its completed record. It repeats this 96 times on one owner, then
shuts down. Every request consumes its acknowledgement; no frame polling or sleeps
are used. The example requires the source-built bootstrap only at build time.

Run from the built runtime directory: `./disvm --no-jit tasks-demo.dis`, then
`./disvm --jit tasks-demo.dis`. Both verify all 96 results and print the same completed/retired task count.
The installed runtime package includes these modules and the full guide above
as this example's README. This service does not implement AS `asinc` closure
lowering or native AS threads. See the full guide for cancellation, stale IDs,
capacity and the actual interpreter/JIT acceptance source.
