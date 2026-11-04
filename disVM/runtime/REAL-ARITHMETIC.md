# Two-phase guarded REAL arithmetic / runtime0.108

ADDF, SUBF, MULF and DIVF use `dvm_runtime_real_arithmetic` in the startup-selected
`--gc-worker --guarded-copies` profile. Default semantics and file opcode IDs stay
unchanged. x64 guarded leaf entries use the existing checked C-tail mechanism.
Other architecture source is retained; physical ARM64/RV64 acceptance is separate.

The borrowed first/second/output spans must be mapped and independently live.
All three complete REAL geometries are checked before input reads or FP effects.
Source pair bytes are captured under one canonical union; input exclusion ends
completely. Native arithmetic evaluates only owner-local representations without
any guards. A second interval protects output byte publication, followed by full
release. Unaligned and overlapping cells work because sources are saved first.
No managed pointer, allocation/free, retain/release, root or callback is introduced.

These are TWO observation points, not a three-cell transaction or atomic shared
read-modify-write. A source can change between snapshot and output publication;
the saved pair remains authoritative. Output admission can fail after FP effects,
leaving output unchanged. Invalid geometry refuses before those effects. Native
FP divide-by-zero/overflow/underflow and native rounding/trap policy are preserved;
there is no integer-style exZdiv substitution. No guard survives FP evaluation or
VM unwind. Error strings are returned after unlock; owning opcode raises them.

Caller still supplies ACTIVE/stopped-world/NativeWorld and frame/referent/GC/code
lifetime. Geometry is not mapping/provenance proof. Guarded competitors must use
the same exclusion domain. RC1 is not a locality test. This family does not solve
shared constructors, FP conversions, graph mutations, retirement epochs or
simultaneous managed mutators. NativeWorld remains required.

## Evidence

The hosted diagnostic links actual matching runtime objects.6912 arbitrary-byte
cases cover four operations and all input/output offsets0..11 in one96-byte
buffer; an independent full-buffer oracle detects overlap/clobber.144 disjoint
masked FP boundary cases use signed zero, signs, DBL_MAX and DBL_MIN. Results and
native exception flags match independent native C expressions:7056 cases total.
This is not exhaustive IEEE payload/rounding or unmasked signal-handler proof.

Actual input/output begin/end wrappers check phase, geometry and independently
calculated guard-union cardinality. Count alone is not identity ownership proof;
the actual canonical guard implementation is used, not replaced. Evaluation
requires zero live guards. A phase-boundary source update preserves the saved
pair. Input/output BUSY refuse without output publication; invalid operation/
output refuse. An actual kprotect/error unwind at unlocked evaluation leaves
zero guards. Allocator wrappers prohibit allocation/free inside the operation.
Four actual-source omissions require exact semantic failures: omitted input
admission, omitted input release, omitted output admission, omitted publication.
Compiler/crash/resource/sanitizer failures never satisfy controls.

Real Limbo C/JIT guests execute all four operations with both collector polls.
Four x64 C-tail tests cover each individual cache unmap, error plus retired code,
return PC, and bad-CALL/old-emitter omissions. TSan positives instrument actual C
paths, not generated machine-code guest instructions. No native many-mutator,
concurrent graph-GC, full GUI or physical-target proof follows.

```sh
python3 disVM/runtime/test_shared_real_arithmetic.py RUNTIME LIMBO INCLUDE_DIR
python3 disVM/runtime/test_real_arithmetic_tail.py RUNTIME LIMBO INCLUDE_DIR
python3 disVM/runtime/benchmark_real_arithmetic.py RUNTIME LIMBO INCLUDE_DIR
```

[Commented Limbo example](../examples/real-arithmetic/README.md).
[Cost ledger](PERFORMANCE-COSTS.md) preserves REALARITH-108 profile medians and
unisolated candidates; it is not an isolated FP/lock/C-tail or IDE speed measure.
