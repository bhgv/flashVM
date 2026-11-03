# Guarded integer conditional branches — runtime0.101

Explicit `--gc-worker --guarded-copies` adds the eighteen existing EQ/NE/LT/LE/GT/GE
branches for BYTE/native WORD/LONG. BYTE uses unsigned ordering, WORD/LONG signed.
Source order remains REG.s compared with REG.m. Complete retained operation bodies
are hashed before startup insertion. Default handlers/emission and Dis64 wire/IDs,
ordinary WORD32 semantics and public execution admission remain unchanged.

## Coherent inputs and conditional targets

`shared_branch.c` first requires the exact ACTIVE service and current owner's PC
slot, validates opcode and both source geometries, then claims one canonical compact
three-span union. For valid target geometry, that union includes the native target
slot even if the eventual predicate is false. Thus native admission/BUSY refusal
can still give exHeap before semantics, including for an unused valid target slot.
For invalid target geometry the third span aliases the first source; invalid
target addresses never become payload reads. No unlock/relock gap exists between
source capture, predicate, used-target capture and PC publication.

`branch_value.c` captures both complete typed values before comparison. Only a
true predicate calls the target reader. Nil/H target pointer contents or invalid
used-slot geometry return exBounds without changing PC. False predicates ignore
those invalid slots/contents. A successful target is copied to the exact owner-
local REG.PC; that output is not arbitrary shared storage. All semantic errors
reach the real opcode error path after guards are released. No allocation, FP
operation, reference transfer, VM callback or blocking occurs under object guards.
Native metadata keeps its existing wait contract. Guards do not establish mapping,
roots or instruction/module/code lifetime. Those remain caller obligations.

## Native and JIT evidence

The actual eighteen C entries check101250 value/alias combinations, complete
payload canaries, both PC outcomes, exact widths and guarded predicate/target
access. Independent controls omit admission/publication or invert predicates;
explicit admission refusal leaves PC/payload unchanged. Additional tests exercise
all three orderings for each operation through the actual native fast dispatcher,
then nil/H/wrapped target slots and nil/H contents for taken/untaken branches,
bad-source precedence, unknown opcode and foreign-PC refusal. Error entry must
have no live guards. No forged Heap headers or synthetic GC roots are used.

The real guest and x64 oracle reach all eighteen actual tail entries. Coverage is
observed between two real Sys print calls; old-emitter omission must fail at the
second call. Each entry has an independent code-unmap case, with inherited C
return PC outside that mapping; bad-source and combined cache/error paths are
checked under both poll modes. Bad-CALL and old-emitter controls must fail
semantically. Compiler/link/signal failure never counts as an omission control.
Generated machine code is not instrumented by TSan.

```sh
python3 disVM/runtime/test_shared_branch.py RUNTIME LIMBO INCLUDE_DIR
python3 disVM/runtime/test_branch_tail.py RUNTIME LIMBO INCLUDE_DIR
python3 disVM/runtime/benchmark_shared_branch.py RUNTIME LIMBO INCLUDE_DIR
```

Developer scripts need the matching native build's object/source closure. TSan
positives use `--positive-only`, which omits deliberate unsafe semantic controls;
it retains full normal case counts, eighteen mappings and errors/both polls.
See [v0.1799](../../docs/releases/v0.1799.md) for exact completed gates and limits.
[Commented example](../examples/guarded-branches/README.md).
[Measured-cost ledger](PERFORMANCE-COSTS.md). Runtime guard selection grows from
48 to66 existing data/branch tails; counts are not a readiness percentage.
NativeWorld still serializes general managed code. FP/string comparisons, other
shared reads/writes, construction, channels/scheduler, root/graph/GC epochs and
frame/module/code lifetime remain before general parallel admission.
