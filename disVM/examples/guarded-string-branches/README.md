# Guarded native string branches

The commented `main.b` compares four ASCII/Rune strings with EQ/NE/LT/LE/GT/GE.
For every pair exactly three predicates hold, giving checksum48. The hosted
fixture separately checks individual predicates against a mathematical oracle;
this aggregate example alone is not a complete comparison proof.

From the checkout (TMPDIR must name an existing writable directory):

```sh
build/runtime/bootstrap/limbo-bootstrap -F -I disVM/upstream/module -o build/string-branches.dis disVM/examples/guarded-string-branches/main.b
build/runtime/disvm --gc-worker --no-jit build/string-branches.dis
build/runtime/disvm --gc-worker --guarded-copies --no-jit build/string-branches.dis
build/runtime/disvm --gc-worker --guarded-copies --jit build/string-branches.dis
```

SDK root: use `./limbo-native -F -I include` with
`examples/guarded-string-branches/main.b`, then those three `./disvm` profiles.
Expected:`PASS string branches6`. Default C is the retained implementation;
explicit guarded C/JIT use continuous source/header/common-prefix exclusion.

H behaves as empty, internal NULL is an error. Headers remain validated even
for identical owners. Only the common prefix is read; Rune keys preserve signed32
ordering, including high-bit patterns. A false condition does not read its target.
Errors release guards before VM propagation; no temporary root, retain or allocation
occurs inside. Caller-owned root/referent/code lifetime and NativeWorld remain
required. This does not enable parallel mutation/building of strings or concurrent
GC. See [STRING-BRANCHES.md](../../runtime/STRING-BRANCHES.md), installed at SDK root.
