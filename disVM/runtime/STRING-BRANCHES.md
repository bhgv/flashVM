# Continuous owner/header/prefix exclusion for string branches

Runtime0.105 protects existing BEQC/BNEC/BLTC/BLEC/BGTC/BGEC under the startup-only
`--guarded-copies` profile, in C and actual x64 tail entries. Default retained
handlers stay unchanged. No shared Dis64 wire/Frame64/feature/ABI ID changes.

The native helper requires exact ACTIVE admission and the current TLS PC address.
It begins one canonical-domain union for both reference cells and a geometrically
valid target cell, then captures both owners. Internal NULL on either side gives
exNilref before header access; H denotes empty. Each live owner adds its len/max
span without releasing earlier claims. Both int fields are captured and checked:
INT_MIN or capacity below the absolute logical count gives exHeap. Identity
comparisons still validate headers. No refcount1 uniqueness assumption is used.

For distinct owners, only the common logical prefix is needed. Checked pointer/
size arithmetic precedes extension of both payload spans. ASCII keys are unsigned
bytes; Rune keys use the retained signed32 mathematical ordering, including all
high-bit patterns. Equal prefixes compare logical lengths. Identical objects and
empty common prefixes read no payload. A true predicate alone snapshots/publishes
the target; false predicates ignore invalid target geometry and sentinel contents.
A geometrically valid unused target can still contribute stripes to the initial
union, matching the existing guarded integer-branch policy.

The source cells, headers and payload prefixes remain excluded through PC
publication. BUSY/refusal returns exHeap only after ending the original access and
all additions; every error leaves PC unchanged. Extension failure itself preserves
its original live prefix, which this handler explicitly ends. No allocation,
retain, temporary root, safepoint, callback or object-owner park occurs inside.
Native metadata mutex fallback retains its existing contract; there is no
wall-clock guarantee. Long prefixes can occupy all256 stripes and serialize
unrelated accesses. No internal yield occurs while guards are live.

Caller-owned mapping/type/root/referent/code lifetimes and collector exclusion
remain independent. Numerical keys and length/capacity checks do not validate an
allocation or grant provenance. Compliant source writers cannot retire captured
owners while source guards stay held. Builders/mutations must acquire compatible
header/payload exclusion before general concurrent use. Existing NativeWorld is
still required; this series does not activate parallel managed code or GC.

`string_branch_value.c/h` contains byte-safe snapshots. The managed handler is a
separate module; it never calls the retained stringcmp/error while guards are
live. Preparation hashes the complete retained native branchstring helper and
refuses drift before source publication. Native JIT selection adds six entries;
69→75 is a local dispatch count, not shared Int32/Ref64 or port readiness.

Actual tests create independent real ASCII/Rune owners and compare all six
conditions against a mathematical signed64 oracle:3888 alignment/representation
cases, including H, allocated empty, mixed representations, prefix lengths and
high-bit Rune32. Precise independent stripe coverage is checked before each
pointer/header/key/target snapshot. A4097-character mixed prefix covers all256
stripes. H/NULL, malformed headers/identities, target geometry/content and foreign
PC/invalid conditions preserve error/PC contracts. Genuine rooted strings survive
32 Cycles before balanced root removal and final Fast. No fake Heap headers or
native-key roots enter the collector. Independent source/extension/publication
omissions require exact semantic failure before unsafe continuation; explicit
BUSY verifies unchanged output and complete prefix release.

Actual x64 entry coverage, individual code unmap, inherited C return-PC,
error/combined and bad-CALL/old-emitter controls exercise all six entries under
both polls. TSan positives instrument C paths; generated machine code remains
uninstrumented. This does not prove physical ARM64/RV64 execution or concurrent
managed mutators. Matching native objects are required for private diagnostic
relinking:

```sh
python3 disVM/runtime/test_shared_string_branch.py RUNTIME LIMBO disVM/upstream/module
python3 disVM/runtime/test_string_branch_tail.py RUNTIME LIMBO disVM/upstream/module
```

`--positive-only` omits deliberate implementation defects for sanitizer positives.
Whole-profile cost/candidates are retained as STRINGCMP-105 in
[PERFORMANCE-COSTS.md](PERFORMANCE-COSTS.md); no isolated predicate/extend/C-tail
cost or error-path timing is inferred. Historical JSON is preserved.

[Commented runnable example](../examples/guarded-string-branches/README.md).
[Remaining managed-heap gates](HEAP-PARALLELISM-STATUS.ru.md).
