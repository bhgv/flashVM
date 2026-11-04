# Guarded list length

The fully commented `main.b` builds32 scalar/pointer-bearing list prefixes and
checks each length, then an empty list. Native LENL keeps its source and every
visited tail excluded until scalar result publication. This requires no temporary
root allocation/retain, but caller-proven referent/root lifetime and current
collector exclusion still apply.

From the checkout, set TMPDIR to an existing writable build directory:

```sh
LIMBO=build/workspace1735/disvm-editor/bootstrap/limbo-bootstrap
RUNTIME=build/workspace1735/disvm-editor/disvm
"$LIMBO" -F -I disVM/upstream/module -o build/list-length.dis disVM/examples/guarded-list-length/main.b
"$RUNTIME" --gc-worker --no-jit build/list-length.dis
"$RUNTIME" --gc-worker --guarded-copies --no-jit build/list-length.dis
"$RUNTIME" --gc-worker --guarded-copies --jit build/list-length.dis
```

SDK root:use `./limbo-native -F -I include` and
`examples/guarded-list-length/main.b`, then the same three `./disvm` selections.
Expected output includes `BEGIN list length` and `PASS list length32`.
Native cells are not ordinary WORD32 Dis or a new shared Frame64 profile.
Actual JIT evidence here is Linux/SysV x64, not physical ARM64/RV64 acceptance.

H terminates an empty/proper list; internal NULL gives exNilref. Brent checkpoints
reject cycles with exHeap. Output is unchanged on failure; native guard contention
also gives exHeap only after complete release. The generic native extension leaves
its original access live on refusal, so the handler explicitly ends it. Visited
links keep the captured reachable path stable against compliant writers. Guards
are not allocation/type validation or permission for uncoordinated GC. NativeWorld
remains required. See [LIST-LENGTH.md](../../runtime/LIST-LENGTH.md), installed at
SDK root, and the [native C example](../native-range-extension/README.md).
