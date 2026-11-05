# Rooted HEADP and TAIL — runtime0.93

`--gc-worker --guarded-copies` now selects rooted pointer-list head/tail projection.
Ordinary selection retains the original HEADP/TAIL bodies. The existing hosted
x64 owner tails enter the actual C operation; guarded HEADP/TAIL refuse held
guards before reading operands. Other guarded JIT backends still refuse this
profile. Explicit C target acceptance remains separate, and NativeWorld still
serializes general managed code.

## Transaction and assumptions

`shared_list_pointer.c` composes the existing [list hold](SHARED-HEAD.md) and
[MOVP transaction](SHARED-POINTER.md). The generator hashes both complete final
native bodies before inserting startup selection. No file opcode or encoding
changes. The new helper takes captured source/destination cells and a trusted
selector (0 for HEADP, 1 for TAIL), not arbitrary guest callback code.

1. Preserve destination geometry refusal before reading the source list. Validate
   current ACTIVE worker role/selector, then prepare a source List hold. Its two
   native root blocks are allocated before guards. Capturing the List retains,
   marks and roots it under the source-slot guard; refusal leaves old ownership
   and destination unchanged.
2. With the container independently rooted, check its actual managed-pool field
   extent before forming an interior source address. HEADP additionally requires
   a stored descriptor with a mapped first pointer slot. A raw byte payload alone
   does not supply a strongly owned pointer. TAIL uses the native List's strong
   tail field, including a valid H terminal value. It relies on well-formed actual
   List/Type ownership, not a new arbitrary-pointer or corrupted-tail class proof.
3. Preserve historical REG.s projection, but pass captured destination to the
   existing MOVP core. MOVP claims the interior field/destination together before
   incoming capture, retain, mark and publication. The List hold supplies the
   containing allocation's lifetime independently of the original reference slot.
4. The MOVP opcode helper roots the displaced output through guard release and
   withdraws that root before consuming the owner. A BUSY interior claim can
   replay the decoded projection after the temporary List hold is released.
   Success and error also finish that hold. A prior projection/destructor error
   survives a later consuming-release error. Committed publication is final.

Source and result may be the SAME cell: ordinary `values = tl values` is one
important case. Publishing a new tail/head owner precedes destruction of the old
List reference. The temporary source root/count remain through consuming callbacks
and disappear only when projection work has returned. REG.s is an operand address,
not an independent root/capability after the opcode finishes.

The caller still independently owns mapped source/destination storage and actual
source referent; geometry/count/Tlist checks alone do not prove provenance. Actual
allocation extent is checked only after that source owner has been captured.
No VM/error/destructor callback spans access guards. Native metadata mutexes can
wait. Additional preparation, field-map/extent and invalid-selector refusals belong
to the explicit profile; no silent fallback or default file policy is introduced.
Arbitrary partial failure inside an unknown destructor is not repaired here.

## Actual evidence

The dedicated fixture uses actual pointer-cons construction, real String/List/Type
owners and external roots, with unaligned native cells. Both HEADP and TAIL run
nonaliased and source/result-aliased cases. It checks both root-preparation
allocation refusals, incoming mark/retain rollback and unchanged old output.
Displaced-output callbacks run only after publication/unlock, remove the original
source owner/root, perform 32 genuine native Cycles passes with only the temporary
parent owner/root, change REG and raise AFTER consuming their current owner.
The first error survives a second error after the temporary List owner is consumed.
Final actual Fast restores String/List/pointer-Type baselines in every scenario.

Edge cases cover H pointer heads, terminal H tails, nil source, destination-first
refusal, invalid source-cell geometry/selector, unmapped raw-head refusal and a
valid tail from a raw List. A compiled guest exercises both actual adapted opcodes.
No Heap or Module header is fabricated.

Seven controls omit container ownership, interior guards, publication, temporary
consumption, displaced consumption, source rooting or incoming rollback. The
publication control changes the actual MOVP source only during this diagnostic;
other controls intercept actual production boundaries. Each must stop at semantic
exit 10 BEFORE unsafe count/GC/retirement continuation. Signals, sanitizer/compiler
errors and timeouts do not count. C/JIT and both polling backends execute them.
An initial fixture check incorrectly treated temporary cleanup after a failed
aliased copy as output retirement; the oracle was corrected to distinguish actual
publication. That test-only failure is not counted as runtime acceptance.

The guarded owner-tail suite checks actual tail coverage, each cache unmap,
nil-source errors, combined cache/error, NULL/foreign/held-guard refusal, bad-CALL
and old-emitter controls. It preserves the process-live C return PC. Generated
machine bytes are not instrumented. Actual pointer-list and guarded-tail positives
also run under TSan. No new hosted-path UBSan, physical ARM64/RV64 or parallel
managed admission acceptance is implied. Final regression counts and measured
whole-profile costs are recorded in the release and performance register.

25 affected console/editor regression tests pass. Actual pointer-list/guarded
owner-tail positives pass TSan. Checkout/SDK examples, installed source closure
and retained 1096 engine/312 bootstrap file pins verify. No new hosted-path UBSan
or target-wide execution proof is inferred.

Local same-image workload, 20000 iterations containing two HEADP and one TAIL,
startup included, warmup plus median of three alternating samples: C default/
guarded 0.123s/0.155s (~26% extra), JIT 0.118s/0.161s (~36% extra). Full profile,
not isolated operations, whole-IDE, cross-target or general statistical evidence.
The persisted LISTP-093 result is in [PERFORMANCE-COSTS.md](PERFORMANCE-COSTS.md).

## Reproduce and remaining work

```sh
python3 disVM/runtime/test_shared_list_pointer.py RUNTIME LIMBO INCLUDE_DIR
python3 disVM/runtime/test_owner_tail.py RUNTIME LIMBO INCLUDE_DIR --guarded-copies
python3 disVM/runtime/benchmark_shared_list_pointer.py RUNTIME LIMBO INCLUDE_DIR
```

Private diagnostic relinks require matching console CMake/GNU-linker objects and
source headers. Instrumented positives accept `--positive-only`; set TMPDIR if
/tmp is unavailable. The full new source/header/test closure is installed under
`source/threading/`. The [commented Limbo example](../examples/guarded-pointer-list/README.md)
runs ordinary C, guarded C and x64 JIT with one native-cell image.

This path prepares two native root blocks per projection. It is a safety baseline;
reuse/local selection remain future optimization work. Confirmed added time is
tracked under LISTP-093 in [PERFORMANCE-COSTS.md](PERFORMANCE-COSTS.md), with a source
marker rather than extra hot-path timing code. Other scalar/raw list reads,
aggregate/list construction, shared arithmetic/reference writers, channels and
scheduler, graph/root/GC epochs and frame/module/code lifetime still need their
protocols before NativeWorld can be removed. No Dis64 wire/feature/ABI ID/layout,
ordinary WORD32 LEA, default file-call admission or Inferno launch/cache agreement
changes follow. No upstream snapshot or Draw implementation is imported.
