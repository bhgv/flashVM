# Guarded HEADB/W/L/F/M — runtime0.94

`--gc-worker --guarded-copies` now covers the five scalar/raw list-head operations.
The same actual C path executes in the interpreter and hosted SysV x64 JIT.
Ordinary selection retains the reviewed previous bodies/emission. Guarded JIT
profiles on other backends still refuse; explicit C target validation is separate.
NativeWorld still serializes general managed execution.

## Algorithm and ordering

`shared_list_bytes.c` captures current native operands once, composes the existing
list hold and allocation-free guarded byte copier, and always finishes temporary
source ownership after normal/error return. The new adapter hashes all five full
final native bodies before adding startup selection. No file opcode/ID changes.

For HEADB/W/L/F, validate destination first using actual BYTE/WORD/LONG/REAL width.
For HEADM, preserve list-before-count ordering: retain/root the source list first,
read the captured native WORD length, reject negative length, return on zero
without consulting destination/payload storage, then validate nonempty destination.
Unreadable length-cell geometry refuses before its read. Length-cell lifetime and
its own access exclusion still rely on the existing VM/caller admission; this
does not introduce a separate concurrent length-operand protocol.

The source List is captured/retained/marked/rooted under its source-slot guard.
Only after capture, check source/destination arithmetic and actual managed-pool
extent for the complete payload. The byte helper then excludes the exact source/
destination byte union, copies with memmove and unlocks. Tiny reads use their
actual width, not a pointer-width overread. Large and overlapping requests use
the same domain as pointer/mapped copies. Source ownership/root survives all
fallible projection work and cleanup. No VM/error callback spans access guards.

REAL projection is bit copying, never floating arithmetic/conversion. Raw copying
transfers no managed referent owner, and does not retire an old managed output.
Correct scalar/raw-versus-reference typing remains a compiler/VM/caller obligation;
use HEADP/HEADMP for reference ownership. The independent container owner and pool
query are essential: numeric geometry/count checks alone do not prove arbitrary
pointer mapping or allocator provenance. Source/destination storage stays owned
by the caller. Zero raw length still requires a valid nonnil source list.

The list hold prepares two native root blocks before guards. Cleanup clears/free
metadata before consuming that extra owner. A body error survives a subsequent
consuming-release error; completed byte publication is not undone/retried.
Arbitrary partial failure in unknown destructors/native code is not repaired.
Native metadata mutexes may wait; allocation-free copying is not wait-free work.

## JIT entries

The existing reviewed x64 guarded-copy emitter now selects ten operations: the
five MOV byte copies and these five HEADs. It checks actual native sizes/REG
offsets and emits ENDBR64/opcode/target/tail-JMP to process-live C. It does not
emit an RX return address surviving callbacks or cache retirement. The C target
declines NULL/foreign REG, unknown opcodes, wrong context/profile and held guards
before operands, then executes the actual opcode once. No decline/retry follows
payload effects or a propagated VM error. Default/other backend emission remains
outside this guarded selection; no shared layout field was added.

## Owning evidence

The diagnostic uses a real cons allocation with 6000 independently patterned raw
bytes and deliberately unaligned native reference/output cells. It covers exact
one-byte/native WORD/LONG/REAL/6000-byte copies, canaries, signaling-NaN bits with
unchanged FP flags, overlapping memmove against a complete independent byte oracle,
actual extent refusal, invalid/negative/zero length and source/destination error
precedence. Source mark refusal and both root allocation refusals restore original
ownership. No fake managed Heap or Module is used.

A diagnostic interposition AFTER the actual byte-copy helper returns forces
native GC while the projection's C frame still owns its list hold. This is not
a production callback API. It removes the original list owner/root, performs 32
genuine native Cycles passes with the temporary hold as sole container owner/root,
changes REG and raises after byte publication. A second consuming-release error
does not replace the first. Final real Fast restores the List Type baseline.
Otherwise successful empty work also propagates its consuming-release error.

Seven actual-boundary controls omit container ownership, payload guards, transfer,
temporary consumption, root publication or rollback, or falsify allocator extent.
Independent checks require semantic exit 10 BEFORE unsafe copy/GC/count use.
Signals, sanitizer/compiler failures and timeouts are not control successes. An
initial overlap fixture used WORD-pointer addition instead of one byte; the
fixture was corrected and rerun. It is not credited as runtime acceptance.

The actual forced-JIT guest reaches all five new emitted entries. The shared ABI
oracle binds actual HEAD opcodes, excluding unrelated MOV tails; production helper/
emitter code is not modeled. Tests retire each opcode's live cache, exercise real
negative HEADM and combined error/cache retirement, NULL/foreign/unknown/held-guard
preflight, both GC polling backends and bad-CALL/old-emitter controls. Existing
MOV-tail tests remain independent. Generated machine bytes are uninstrumented.
Actual C/JIT/both-poll and TSan evidence is recorded in this release; no new
physical ARM64/RV64, hosted-path UBSan or parallel managed acceptance is implied.

## Reproduce and remaining work

```sh
python3 disVM/runtime/test_shared_list_bytes.py RUNTIME LIMBO INCLUDE_DIR
python3 disVM/runtime/test_list_bytes_tail.py RUNTIME LIMBO INCLUDE_DIR
python3 disVM/runtime/benchmark_shared_list_bytes.py RUNTIME LIMBO INCLUDE_DIR
```

Private hosted tests need matching console CMake/GNU-linker objects; instrumented
positives accept `--positive-only`. Set TMPDIR if /tmp is unavailable. The new
source/header/test closure is installed under `source/threading/`. The commented
Limbo example uses the same image for ordinary C, guarded C and x64 JIT.
Measured added cost is tracked in [PERFORMANCE-COSTS.md](PERFORMANCE-COSTS.md),
not hidden or summed with unrelated workloads. Root preparation/reuse and compiler
local/shared selection remain optimization work.

List construction, other shared arithmetic/reference writers, channel/scheduler
transfers, graph/root/GC epochs and frame/module/code lifetime still prevent
removing NativeWorld. No Dis64 wire/feature/ABI ID/layout, ordinary WORD32 LEA,
default file admission or Inferno named launch/cache agreement changes. Retained
upstream stays pinned; no Draw implementation or peer execution proof is imported.

Release validation: 27 affected CTests pass in 172.12 seconds. Both new actual
C/JIT suites pass TSan positives. Installed root-registry/batch tests and all
three installed example startup profiles are checked separately.
