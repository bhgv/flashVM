# Guarded native shifts

`main.b` checks six Limbo-emitted operations: SHL/SHR for byte, int and big.
Counts have native WORD width even for byte. Byte right shift inserts zero;
negative int/big right shift inserts sign bits. Left shifts retain modular wrap.
Every result is checked against multiplication/division with bounded counts.

From a source checkout (set TMPDIR to an existing writable build directory):

```sh
LIMBO=build/workspace1735/disvm-editor/bootstrap/limbo-bootstrap
RUNTIME=build/workspace1735/disvm-editor/disvm
"$LIMBO" -F -I disVM/upstream/module -o build/guarded-shifts.dis disVM/examples/guarded-shifts/main.b
"$RUNTIME" --gc-worker --no-jit build/guarded-shifts.dis
"$RUNTIME" --gc-worker --guarded-copies --no-jit build/guarded-shifts.dis
"$RUNTIME" --gc-worker --guarded-copies --jit build/guarded-shifts.dis
```

Installed runtime/editor SDK: use `./limbo-native -F -I include` with
`examples/guarded-shifts/main.b`, then the same three `./disvm` selections.
Expected output: `PASS guarded shifts: six emitted operations`.

This image uses native cells, not ordinary WORD32 Dis or a new shared Dis64 ABI.
The runtime also guards two existing logical LSR opcodes, tested through actual C
entries/native dispatch; this source does not claim to emit them. Invalid counts,
aliased slots and error precedence have separate developer fixtures documented in
[SHARED-SHIFTS.md](../../runtime/SHARED-SHIFTS.md), installed at SDK root.
Guards require caller-proven live mapped cells; general managed execution still
uses NativeWorld. This example does not claim parallel shared-heap admission.
