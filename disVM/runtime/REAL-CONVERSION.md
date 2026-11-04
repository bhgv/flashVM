# Two-phase FP conversion / runtime0.110

CVTRF/FR/WF/LF/FW/FL use `dvm_runtime_real_conversion` in the explicit startup
`--gc-worker --guarded-copies` profile. The default retained paths and existing
opcode IDs are unchanged. Source/output widths follow native SREAL/REAL/WORD/
LONG, not Dis64 wire Int32 inference. Public module/wire/native-extension layouts
are unchanged. Source adaptation checks complete retained operation hashes.

Both complete geometries preflight before any input/FP effects. A source-only
canonical range protects byte capture. All guards end before native conversion
of saved local representations. Success alone enters a separate output-only
range and publishes bytes. Unaligned/overlapping cells are valid because capture
precedes publication. No allocation/free, retain/release, roots or callbacks are
inserted inside the helper. Static diagnostics return only after unlock; the
owning opcode performs VM error propagation after helper return.

REAL-to-integer preserves the retained half-away adjustment followed by native
truncation/range checks. NaN and out-of-range results return exInval before output
admission/publication. WORD32 and native64 retain their distinct bound predicates;
only native64 is executed locally in this series. Narrowing/widening real and
integer-to-real keep the current native FP status/rounding/trap policy. There is
no integer divide-zero substitution and no unmasked-signal acceptance claim.

These are TWO observation points, not atomic shared RMW or a source/output
transaction. Source updates after capture do not alter the saved value. Output
admission refusal may follow FP effects while preserving output. Caller still
supplies independently mapped/live cells, ACTIVE/world admission, frame/referent/
code/GC lifetime, shared-domain participation and FP environment. Address geometry
is not provenance or mapping proof; RC1 is not locality evidence. NativeWorld
remains required. Graph mutation/GC epochs, channels/scheduler, constructors and
code/module lifetime are outside this family.

## Evidence and limits

Actual matching runtime objects execute3456 arbitrary-byte alias/unaligned cases:
six conversions,24 source offsets by24 output offsets, independent96-byte whole
buffers. A retained-source differential baseline changes only argument plumbing
and static error returns; complete operation hashes are pinned. This is not an
independent language specification. Independent known tests add16 half-value
rounding outcomes and8 invalid rounded inputs;16 binary32/64 IEEE boundary cases
add signed zeros, infinities, quiet/signaling NaNs, finite maximum and subnormal.
Total3496 cases compare results, output conservation and native FP flags with
masked traps. This is not exhaustive IEEE payload/rounding, native WORD32 or
physical-target acceptance.

Actual begin/end/copy/evaluation hooks check source-only/output-only geometry,
independent guard-union cardinality, full release and byte counts. Cardinality
alone is not exact stripe-ownership proof; actual canonical exclusion is used.
Saved-value source update, input/output BUSY, invalid opcode/output, allocator
prohibitions and actual unlocked kprotect/error unwind pass. Four private actual
helper omissions require exact failures: missing input admission/release/output
admission/publication. Crash/compiler/sanitizer/resource failures never count.

All six actual C dispatcher entries return1 with exactly one genuine helper
call. A real Limbo guest emits FOUR CVTWF/LF/FW/FL paths, in C/JIT with both
collector polls. Four actual x64 tails additionally cover individual cache unmap,
error with retired code, return PC, bad-CALL/old-emitter and twelve per-opcode
actual-dispatch omissions (decline/false success/double execute). CVTRF/FR have
actual C dispatch and source-selector evidence, not emitted guest execution proof.
The bounded generated-selector/source-dispatch audit now covers91 entries with
274 exact source controls; that does not execute91 operations or grant ownership.
TSan positives instrument actual C, not generated machine-code instructions.

```sh
python3 disVM/runtime/test_shared_real_conversion.py RUNTIME LIMBO INCLUDE_DIR
python3 disVM/runtime/test_real_conversion_tail.py RUNTIME LIMBO INCLUDE_DIR
python3 disVM/runtime/test_guarded_dispatch.py MATCHING_BUILD/native
```

The differential diagnostic uses the retained executor beside INCLUDE_DIR, or
the byte-equal bundled source/conversion-oracle/xec.c in the installed SDK. That
file is an oracle input, not an independent VM import/build dependency. Matching
compiled runtime objects and flags are required for private relink diagnostics.
SDK includes source, tests, guide, raw performance samples and a
[fully commented Limbo example](../examples/real-conversion/README.md).

[REALCONVERT-110 cost and unisolated candidates](PERFORMANCE-COSTS.md).
