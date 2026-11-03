# Guarded scalar and raw list heads

The fully commented Limbo program checks byte, int, big, real and a pointer-free
record. These select HEADB/W/L/F/M. A String-bearing record instead requires
HEADMP and managed ownership; raw copying is not a substitute for it.

Compile once, then compare ordinary and explicit guarded execution:

```sh
LIMBO=build/workspace1735/disvm-editor/bootstrap/limbo-bootstrap
RUNTIME=build/workspace1735/disvm-editor/disvm
"$LIMBO" -F -I tools/limbo-bootstrap/upstream/module \
  -o build/guarded-list-bytes.dis disVM/examples/guarded-list-bytes/main.b
"$RUNTIME" --gc-worker --no-jit build/guarded-list-bytes.dis
"$RUNTIME" --gc-worker --guarded-copies --no-jit build/guarded-list-bytes.dis
"$RUNTIME" --gc-worker --guarded-copies --jit build/guarded-list-bytes.dis
```

From an installed runtime/editor SDK root:

```sh
./limbo-native -F -I include -o guarded-list-bytes.dis examples/guarded-list-bytes/main.b
./disvm --gc-worker --guarded-copies --no-jit guarded-list-bytes.dis
./disvm --gc-worker --guarded-copies --jit guarded-list-bytes.dis
```

Expected: `PASS guarded scalar lists: 137 731 9000000007 3.5 3 7`.
The paths illustrate one local build. Guarded JIT acceptance currently covers
hosted SysV x64; other targets need independent testing. The image is native-cell,
not an ordinary legacy Inferno file or a new Dis64 wire encoding.

The guarded driver roots and retains the actual source List before deriving its
payload address, checks the actual allocation extent, then copies exact bits
under ordered source/destination guards. Temporary ownership is cleaned on every
error. No floating arithmetic is introduced, including for signaling NaN payloads.
The diagnostic suite additionally covers unaligned/overlapping buffers, wide
payloads, refusal ordering and actual collection during protected cleanup.

See [the complete contract](../../runtime/SHARED-LIST-BYTES.md), installed as
SHARED-LIST-BYTES.md at SDK root. These protections do not remove NativeWorld or
prove general parallel managed execution; construction, other writers, channels,
root epochs and executable lifetimes remain separate work.
