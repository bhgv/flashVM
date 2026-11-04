# Guarded native array length

`main.b` checks32 lengths for byte/int/string arrays and a nil array. The source
is fully commented. Mapped string elements initialize to nil; every array remains
owned by its ordinary local reference. Both allocated-empty and nil arrays report0.

From the checkout, set TMPDIR to an existing writable build directory:

```sh
LIMBO=build/workspace1735/disvm-editor/bootstrap/limbo-bootstrap
RUNTIME=build/workspace1735/disvm-editor/disvm
"$LIMBO" -F -I disVM/upstream/module -o build/array-length.dis disVM/examples/guarded-array-length/main.b
"$RUNTIME" --gc-worker --no-jit build/array-length.dis
"$RUNTIME" --gc-worker --guarded-copies --no-jit build/array-length.dis
"$RUNTIME" --gc-worker --guarded-copies --jit build/array-length.dis
```

SDK root:use `./limbo-native -F -I include` and
`examples/guarded-array-length/main.b`, then the same three `./disvm` selections.
Expected output includes `BEGIN array length` and `PASS array length32`.
The image uses native cells, not ordinary WORD32 Dis or a shared Frame64 profile.
Actual JIT evidence here is Linux/SysV x64, not physical ARM64/RV64 acceptance.

The guarded handler continuously protects the existing source owner while it
captures the reference, try-claims the discovered length field and publishes the
WORD result. It allocates no temporary roots/owners and releases all guards before
error propagation. Failed extension preserves its original access until explicit
end. Caller-proven cell/referent lifetime and current GC/root exclusion remain
required; this does not remove NativeWorld or enable concurrent graph collection.
See [ARRAY-LENGTH.md](../../runtime/ARRAY-LENGTH.md), installed at SDK root, and the
[C example](../native-small-extension/README.md) for the native primitive.
