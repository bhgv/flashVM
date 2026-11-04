# Coherent REAL input pair, unlocked predicate, later guarded target

Runtime0.106 selects the six existing BEQF/BNEF/BLTF/BLEF/BGTF/BGEF handlers
under startup-only `--guarded-copies`, in C and actual x64 tails. The retained
default profile is unchanged. There is no new wire/feature/ABI/opcode ID.

The exact ACTIVE owner and its TLS PC are checked before operands. Both complete
REAL cells form one canonical-domain union; capture copies representations only
into owner-local byte storage. The entire union and native domain borrow end
before evaluating the native C floating-point predicate. No shared bytes are
read during that predicate. Native NaN/unordered/signed-zero/FP-environment
behavior is retained; the production helper neither masks nor clears FP status.

Only a true predicate validates, claims and snapshots the target pointer under
a new interval, then publishes the current owner's PC. A false predicate does
not even claim target stripes. Invalid used geometry/sentinels give exBounds;
refused native admission gives exHeap. All static errors return after unlock,
with PC unchanged. Unknown condition/foreign PC refuse before payload access.
No allocation, reference transfer or temporary root is introduced.

**Two observation points:** the pair is coherent at capture, but the taken target
is sampled later. There is no atomic transaction over all three cells. Input
writers may change original cells after capture without changing the saved
predicate; a target writer may publish a new pointer before the later target
interval. Compiler branch targets ordinarily remain stable, but callers must
still prove target/frame/code/mapping lifetime through both phases. No lock is
held across native FP evaluation or its possible error/unwind boundary.

This deliberately differs from integer/string guarded predicates that can remain
inside their shared-memory interval without FP exceptions. Do not move FP into
that interval merely to imitate their structure. Native metadata mutex fallback
retains its existing policy, with no wall-clock guarantee. Guards grant no
provenance, rooting or concurrent collector permission. General managed execution
still requires NativeWorld; arithmetic/conversion, graph/GC, channels and code
lifetime remain separate protocols.

`real_branch_value.c/h` separates byte capture from local evaluation. The source
preparer hashes the complete retained branchreal helper and refuses drift. Six
native x64 selections increase the local data/branch-tail count81 from75; this
is not shared Frame64 Int32/Ref64 or whole-port readiness.

Actual matching-object tests cover135600 cases:135000 overlapping offset
cases plus600 disjoint cases preserving all ten binary64 patterns intact.
All six conditions are checked. Inputs include both zeros,
positive/negative infinity, quiet/signaling NaN, minimum subnormal and maximum
finite values. The boolean oracle compares representations mathematically with
no FP instructions; FP status is independently compared with current native C
operators. The fixture saves/restores the full FP environment and masks traps
for these special-value cases. This is not a test of unmasked hardware-signal
handling. A separately injected real VM error at the unlocked predicate boundary
verifies normal kprotect cleanup without claiming signal-handler acceptance.

Independent read coverage checks the input union and later target. Four omission
controls catch missing initial admission, missing input release before FP,
missing target admission and omitted PC publication at exact semantic stages.
Input/target BUSY preserve PC/canaries. A dedicated phase-boundary update changes
both original operands and target after capture: saved equality still holds and
the new target is published. Lazy target geometry/content, invalid source,
condition/foreign PC and allocation-free cleanup are checked under both polls.
No fake Heap or native-key GC root is created; saved REAL bytes need no owner.

All six actual x64 entries pass coverage, individual code unmap, inherited C
return-PC, error/combined and bad-CALL/old-emitter controls. TSan positives
instrument C/native paths, not generated machine code. No physical ARM64/RV64,
parallel managed mutator, full editor or unmasked SIGFPE proof follows.

```sh
python3 disVM/runtime/test_shared_real_branch.py RUNTIME LIMBO disVM/upstream/module
python3 disVM/runtime/test_real_branch_tail.py RUNTIME LIMBO disVM/upstream/module
```

`--positive-only` omits deliberate defects but retains refusal/error/phase-update
controls. Whole-profile cost and open candidates are recorded as REALBRANCH-106
in PERFORMANCE-COSTS.md; error/trap costs and individual component shares are not
isolated. Historical JSON stays unchanged.

[Commented example](../examples/guarded-real-branches/README.md).
[Remaining managed-heap gates](HEAP-PARALLELISM-STATUS.ru.md).
