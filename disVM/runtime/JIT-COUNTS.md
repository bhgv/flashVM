# Pointer-list ownership in mixed JIT — runtime0.73

Hosted runtime now declines the unchecked `LeafPointerCons` machine fast path
before allocation or publication. Its existing C `consp` opcode uses checked
Heap/Type retain helpers and the existing marking/root protocol. Other leaf
kinds remain eligible for native lowering. Serialized managed admission is
still required; this does not complete concurrent payload/GC protocols.

The policy is confined to the generated hosted `leaf64-ops.h` validator.
Its reviewed source body hash must match before adaptation; source changes or
double adaptation refuse. The x64/ARM64 emitters already consult this common
validator. RV64 does not acquire a new lowering through this policy. Pinned
upstream and standalone emitter fixtures remain unchanged. No LeafLayout ABI,
serialized opcode ID, Dis64 field or native pointer on wire is introduced.

This is a temporary performance tradeoff for pointer-list construction, not a
claim of faster list allocation. Restore direct lowering only with checked
native count RMWs or a proven equivalent per-target protocol. Source slot
lifetime, zero/overflow refusal, allocation failures, register/stack roots and
marking remain separate obligations. A LOCK prefix alone is insufficient.
This policy addresses the identified pointer-cons path, not every generated
machine-code access or COW uniqueness decision.

`test_jit_counts.py RUNTIME COMPILER MODULE_INCLUDES PINNED_LEAF_LAYOUT` relinks
private console binaries against matching objects. A real Limbo guest builds
64 string-reference nodes between native Sys gates. All64 Tptr acquisitions
must reach the checked helper in C and mixed JIT, with both polling modes.
Native instruction statistics must remain positive in JIT and zero in C mode.
Payload/list length and source-drift refusal are also checked.

The x64 negative control recompiles only a private backend object with the old
validator restored. It must detect bypassed owners at the exact status10
predicate before continuing the guest. Production sources/objects are untouched.
Compilation failures, signals or timeouts never count as semantic detection.
The private relinker requires matching GNU/Makefiles metadata. Actual x64
emitted instructions run in this control; they are not TSan-instrumented.
ARM64 uses the shared policy but has not been executed locally in this series.
