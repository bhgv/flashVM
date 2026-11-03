# Guarded products, quotients and remainders — runtime0.98

Explicit --gc-worker --guarded-copies now includes MUL/DIV/MOD for BYTE, native
WORD and LONG. The existing guarded arithmetic set grows from 15 to 24 operations.
These are existing opcodes, not new shared Dis64 IDs. The complete final bodies
are hashed before startup selection insertion; default semantics stay intact.

## One snapshot, exception precedence

MUL captures both sources under the same compact three-span union as add/sub/
bitwise operations, computes modulo native width and publishes before unlock.
BYTE promotion fits int; WORD/LONG products use unsigned types to avoid signed
C overflow. No allocation, reference ownership change, FP or VM callback occurs
while guards are held. Caller independently owns live mapped spans/root lifetime.

DIV/MOD first preflight owner/opcode and valid divisor geometry. For valid outer
spans, claim all three together, read divisor, then capture dividend and compute.
The divisor stays stable between its zero check and evaluation. There is no
read-unlock-evaluate-relock gap. Signed quotient truncates toward zero; remainder
follows dividend sign. Existing MIN/-1 behavior is retained: quotient MIN,
remainder 0, without evaluating the overflowing C expression.

For nil/H/wrapping dividend/output geometry, claim only the valid divisor through
three aliased spans. Zero yields exZdiv, otherwise exBounds. Invalid outer payloads
are never read or written. Semantic errors return only after releasing all guards;
the actual opcode then raises the VM error. Native admission/BUSY refusal still
precedes semantic evaluation and returns exHeap before effects. Unmapped arbitrary
pointers are not made safe by geometry/guards; mapping/provenance remains external.

## Evidence and reproduction

The actual C fixture covers all 24 operations through 1404000 value/source/output
alias cases, with a separate unsigned-magnitude signed-division oracle. Zero cases
require unchanged complete canaries, exact exZdiv and no held guards. Dedicated
MIN/-1 cases cover WORD/LONG DIV/MOD. Zero versus invalid outer sentinels and
nonzero versus invalid outer errors are tested separately. Every divisor read
is observed under guards; every actual VM error entry is observed after unlock.
Three strict controls omit admission/publication or replace the zero diagnostic.

The real Limbo guest reaches all 24 x64 tails, including each mapping unmap and
return-PC ABI. Combined cache/error controls now invoke actual DIVL with zero
through the real checkpoint after mapping retirement. Six actual failing guests
(DIV/MOD for three types) require exit 1/zero-divide, no post-result print and
identical default/guarded C/JIT behavior. Generated machine bytes remain
uninstrumented; no whole parallel managed or new physical target proof follows.

```sh
python3 disVM/runtime/test_shared_arithmetic.py RUNTIME LIMBO INCLUDE_DIR
python3 disVM/runtime/test_arithmetic_tail.py RUNTIME LIMBO INCLUDE_DIR
python3 disVM/runtime/test_arithmetic_errors.py RUNTIME LIMBO INCLUDE_DIR
python3 disVM/runtime/benchmark_product_division.py RUNTIME LIMBO INCLUDE_DIR
```

Private wrappers require matching GNU/CMake console objects. Positive-only hosted
sanitizer suites accept --positive-only; failing-guest tests always check exact
failure. Set TMPDIR if /tmp is unavailable. The installed SDK includes source/test
closure and the [commented Limbo example](../examples/guarded-product-division/README.md).
Historical fifteen-op benchmark guest bytes are frozen using FAMILIES[:5], so
new coverage does not silently change the old workload. Measurements remain in
[PERFORMANCE-COSTS.md](PERFORMANCE-COSTS.md), never summed across distinct workloads.

NativeWorld still serializes general managed execution. Remaining reads/writers,
shifts/FP and comparisons, list construction, channel/scheduler ownership,
graph/root/GC epochs and frame/module/code lifetime remain before global admission
can be removed, followed by compiler-proven local/shared paths. Native WORD64
semantics do not define Inferno Frame64 Int32/Ref64 wire rules. No default 43/173,
ordinary WORD32 LEA, named immutable launch/cache or vendor pin change.

Release validation: 39 affected CTests pass in 261.62 seconds. TSan positives pass
actual arithmetic, all 24 emitted tails and six failing-guest variants. Installed
source/test closure and all three example profiles pass. Vendor pin verifies 1096
DisVM / 312 bootstrap files. Historical 15-op guest bytes match the previous version.
No new hosted UBSan/physical ARM64/RV64 or parallel managed-program proof inferred.

MULDIV-098 local whole-profile cost: C 4.038584918→4.380780999 seconds (+8.47%);
JIT 2.460638658→4.385723005 (+78.24%). Same binary/image, warmup then median of
three startup-inclusive runs, 20000 iterations of 24 explicit operations plus checks/
loop/conversions. Divisors stay nonzero. These figures do not isolate a helper,
measure error latency or add to prior 15-op workload percentages.
