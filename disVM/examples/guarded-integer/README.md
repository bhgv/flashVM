# Guarded integer conversion example

The commented Limbo program checks every byte value through BYTE→WORD→LONG→WORD.
Together with the narrowing cast it exercises CVTWB/BW/WL/LW. Native SHORT
conversions are tested by the six-op runtime fixture; Limbo has no short source
type in this example. All operations use native runtime WORD width, not a new
file-format word size.

Compile once from a checkout, substituting your matching build paths:

```sh
LIMBO=build/workspace1735/disvm-editor/bootstrap/limbo-bootstrap
RUNTIME=build/workspace1735/disvm-editor/disvm
"$LIMBO" -F -I tools/limbo-bootstrap/upstream/module \
  -o build/guarded-integer.dis disVM/examples/guarded-integer/main.b
"$RUNTIME" --gc-worker --no-jit build/guarded-integer.dis
"$RUNTIME" --gc-worker --guarded-copies --no-jit build/guarded-integer.dis
"$RUNTIME" --gc-worker --guarded-copies --jit build/guarded-integer.dis
```

From the installed runtime/editor package root:

```sh
./limbo-native -F -I include -o guarded-integer.dis examples/guarded-integer/main.b
./disvm --gc-worker --guarded-copies --no-jit guarded-integer.dis
./disvm --gc-worker --guarded-copies --jit guarded-integer.dis
```

Expected: `PASS guarded integer: all 256 byte values`.
These are native-cell images; ordinary legacy Dis admission is unchanged.
Guarded JIT acceptance here is Linux/SysV x64. Other targets require independent
validation. The conversion helper does not allocate managed/native metadata or
call VM services while holding guards; it does not acquire ownership or prove
that arbitrary input addresses are mapped. The caller still supplies live cells.

See [SHARED-INTEGER.md](../../runtime/SHARED-INTEGER.md), installed at package
root, for overlap/error ordering and coverage. NativeWorld is still required;
this is one protected writer cohort, not general parallel managed execution.
