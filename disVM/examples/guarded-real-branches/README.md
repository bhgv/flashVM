# Guarded REAL branches

The commented example compares four dynamic REAL values with EQ/NE/LT/LE/GT/GE,
then checks checksum48. The actual-runtime fixture independently checks135600
special-value/alias cases; this small aggregate example alone is not that proof.

From the checkout, set TMPDIR to an existing writable directory:

```sh
build/runtime/bootstrap/limbo-bootstrap -F -I disVM/upstream/module -o build/real-branches.dis disVM/examples/guarded-real-branches/main.b
build/runtime/disvm --gc-worker --no-jit build/real-branches.dis
build/runtime/disvm --gc-worker --guarded-copies --no-jit build/real-branches.dis
build/runtime/disvm --gc-worker --guarded-copies --jit build/real-branches.dis
```

Installed SDK root: compile `examples/guarded-real-branches/main.b` with
`./limbo-native -F -I include`, then use the same three `./disvm` selections.
Expected:`PASS real branches6`.

The input pair has one coherent guarded snapshot. Floating-point evaluation
happens after complete release, preserving native FP behavior. A true condition
alone samples its target under a new guard. Inputs and target have separate
observation points: this is not a three-cell atomic transaction. Caller-owned
frame/code/mapping lifetime and NativeWorld remain required. No allocation,
temporary root or reference transfer occurs inside either interval. See
[REAL-BRANCHES.md](../../runtime/REAL-BRANCHES.md), installed at SDK root.
