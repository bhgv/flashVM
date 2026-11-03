# Invocation-local stack checking — runtime0.81

The retained hosted `memchk` diagnostic previously used one process-global
`static int depth` to cap recursive graph traversal. Independent calls shared
that counter, so a nested diagnostic inherited an already consumed budget;
future concurrent callers would also race on it. The hosted adaptation replaces
it with a value parameter on a private recursive helper. Public `memchk(p,t)`
starts a fresh budget. No TLS field, Heap header, public signature or wire layout
changes. Root depth0 through100 preserves the original maximum101 visited
nodes; an entry at depth101 returns before graph access.

The diagnostic's direct live Heap.ref observation now uses the checked native
atomic load. That count observation still provides no lifetime acquisition,
root, payload permission or COW guarantee. `p`, its Type and all traversed graph
objects must remain independently live and protected by existing managed/GC
admission. Type maps, payload pointers and mark state are not made atomic by this
change. The recursion limit bounds cycles, not the work of an arbitrary large
branching graph. This remains a diagnostic, not a concurrent graph verifier.

`prepare_stack_check.py` checks the complete reviewed native-register source
hash and each exact replacement boundary. Pinned upstream bytes stay unchanged;
source drift and repeated conversion refuse. Errors/abort remain the retained
diagnostic's terminal policy; there is no callback or recoverable rollback API.

## Reproduce with matching native build

`python3 disVM/runtime/test_stack_check.py RUNTIME LIMBO INCLUDE` relinks private
actual validstk objects against matching GNU Makefiles console build metadata.
It allocates a REAL managed pointer-shaped object/Type, acquires a distinct
self-reference and registers its Heap root. A diagnostic-only hmsize interception
reenters the actual checker on that same protected cyclic graph. Both nested
and outer traversals must observe101 actual hmsize checks, twice in succession.
The real hmsize validation still runs. No payload mutation or GC/admission wait
occurs inside either traversal. Afterward the cycle is explicitly broken,
both owners/root and Type creator retire, actual Fast completes, and the normal
root/Cycles and managed-owner probes plus ordinary Dis execution continue.

Two private actual-source controls restore shared-depth behavior or remove the
recursion bound. Each requires exact semantic10 at the budget predicate; an
unbounded traversal stops before the102nd actual size check, not at a stack
fault. Compiler/signal/timeout/sanitizer failures never count as detection.
C/JIT and both polling backends run. `--positive-only` excludes deliberate source
controls for instrumented builds. This proves reentrancy/budget/actual ownership
integration under managed exclusion, not concurrent arbitrary managed access.

The source SDK includes adapter, diagnostic and documentation. Private relinking
needs the matching console build's objects/flags; the SDK executable alone does
not provide that build metadata.
