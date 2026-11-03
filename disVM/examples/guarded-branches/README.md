# Guarded typed conditional branches

The fully commented `main.b` exercises EQ/NE/LT/LE/GT/GE for byte, int and big.
Runtime arrays keep values dynamic. Byte compares unsigned; negative int/big use
signed ordering. Each pair satisfies three predicates, so eight rounds across
three widths must score72. The actual native fixture checks individual predicates
and aliases independently; this aggregate guest is additional execution evidence.

From the checkout, set TMPDIR to an existing writable build directory:

```sh
LIMBO=build/workspace1735/disvm-editor/bootstrap/limbo-bootstrap
RUNTIME=build/workspace1735/disvm-editor/disvm
"$LIMBO" -F -I disVM/upstream/module -o build/guarded-branches.dis disVM/examples/guarded-branches/main.b
"$RUNTIME" --gc-worker --no-jit build/guarded-branches.dis
"$RUNTIME" --gc-worker --guarded-copies --no-jit build/guarded-branches.dis
"$RUNTIME" --gc-worker --guarded-copies --jit build/guarded-branches.dis
```

Installed SDK:use `./limbo-native -F -I include` and
`examples/guarded-branches/main.b`, followed by the same three `./disvm` selections.
Expected output includes `BEGIN branches`, `PASS branches18` and application
completion. Actual JIT evidence here is Linux/SysV x64, not physical ARM64/RV64.

The guard spans both inputs and valid native target geometry together. Target
payload is read only when the condition is true. False conditions ignore invalid
target slots/contents; admission/BUSY may still refuse before evaluating semantics.
Taken invalid targets raise exBounds after unlock, preserving PC. This native-cell
example is not ordinary WORD32 Dis or a new shared Dis64 ABI. Guards do not keep
instruction/module code alive; NativeWorld and caller-proven lifetimes remain.

[Full contract and developer controls](../../runtime/SHARED-BRANCHES.md), installed
at SDK root. This example uses no spawn and does not demonstrate general parallel
shared-heap admission. The separate native range/owner suites test OS concurrency.
