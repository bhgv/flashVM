# Guarded scalar/raw copies and SELF — runtime0.89

Runtime0.91 also adds opt-in mapped MOVMP: [SHARED-MAPPED.md](SHARED-MAPPED.md).

`MOVB`, `MOVW`, `MOVL`, `MOVF` and `MOVM` have explicit checked byte-range paths.
Select `--gc-worker --guarded-copies` before loading a native-cell module. Both
C and hosted SysV x64 JIT use the guarded profile. It is experimental and DOES
NOT remove NativeWorld or enable parallel managed heap admission. Default scalar
copies/native emission remain the normal path until compiler local/shared
selection and the remaining protocols are ready. Other backends refuse this
profile with JIT; use their C path explicitly, with target validation still needed.

`SELF` independently uses the existing checked MOVP transaction: current native
REG.M is context-owned/rooted during the opcode; destination publication is under
cell guards, and displaced-module destruction happens after unlock. Preserve its
original destination-first and nil-module exception precedence. Other list/source
container projection/lifetime work is not claimed as completed by this change.

## Native algorithm and interfaces

Small spans 1..sizeof(void*) extend the unchanged pointer-cell API with exact byte
lengths. No extra mapped bytes beyond a scalar are required. Wide spans use a
separate `DVM_RangeAccess`, keeping its 256-pointer metadata out of the hot small
copy stack. Both APIs use the SAME existing native domain/stripe identities, so
partial overlap between byte, scalar and pointer access is excluded. The original
pointer-access struct/signature remains unchanged. No Heap/Type or file layout
changes follow from these native metadata structures.

A fixed bitset selects the stripe union. Empty spans ignore their addresses;
a successful empty access still pins domain lifetime until end. Contiguous spans
covering a period select all 256 stripes without looping over unbounded input
length. Smaller spans mark actual boundary cells, including modulo wrap. Claim
in ascending unique stripe order; on BUSY/refusal release every acquired prefix
and native borrow before returning. No per-call allocation or object-owner wait.
Metadata fallback locks may block. Collisions can serialize independent data.
Large copies can hold all stripes for their duration; no latency/yield guarantee
is made. Caller independently owns mapped ranges/domain/pointed-to lifetimes.

Both access records are zero-before-begin, owner-local and noncopyable. End checks
exact TLS identity and held depth before mutation. Creator excludes new lookups
and externally quiesces users before domain destruction; a borrow count cannot
protect its own first lookup. No VM/error callback, native wait, owner/world
transition or arbitrary unwinding may span an access. Generic byte guards grant
no managed retain, root, mapping/provenance or payload authority to bare code.

## Hosted implementation

Six reviewed final opcode bodies are SHA256-bound after prior native/count views.
`prepare_shared_bytes.py` preserves MOVM's captured WORD length, signed check,
zero-length/no-data behavior and exception precedence. Its length operand and
module/type lifetimes still rely on existing admission/caller protection; they
are not independently made parallel by guarding payload alone. Byte copying is
bit-preserving memmove, not typed reference assignment: MOVMP/HEADMP ownership
is a separate unfinished transaction, never inferred from raw bytes.

Shared native role preflight moved into `shared_access.c`, reused by MOVP and
bytes. Raw-transfer implementation is a separate generic C module. No callback
or exception occurs under guards. Refused/busy copies return an error before
payload effects; the opcode raises after cleanup. No unsafe unguarded fallback
or retry after success. Guarded copy selection is startup-only before loading/
JIT publication, never changed while code/workers exist, not inferred from bytes.
Duplicate selection and incompatible inline mode refuse explicitly.

The x64 adapter hashes the complete reviewed leaf emitter and selects exactly
five opcodes only for the guarded profile. It binds actual native WORD/REAL/LONG
sizes and REG offsets, emits ENDBR64/MOV opcode/MOV target/tail JMP and preserves
the original process-live C return PC. Native preflight refuses NULL/foreign REG,
unknown opcodes and held guards before operands. C errors and cache-retirement
callbacks cannot return through an unmapped JIT scaffold. Other opcodes, default
emission and other backend emitters remain unchanged. This is a checked C-tail
baseline, not an inline machine-copy acceleration claim.

## Validation

Native and forced-lock tests mix eight OS writers and 16000 paired updates through
small-cell and wide-range APIs over the SAME data. Exact one-byte boundaries,
empty borrows, modulo wrap, full-domain selection, ordering and cleanup are
checked. Fail-first native allocation monitoring proves initialized intervals
make no allocation attempt. Eight actual-source controls require semantic exit 10
before unsafe continuation. Existing 66 constructor-refusal/cell tests still pass.

Actual C/JIT/both-poll diagnostics cover six copy sizes through 6000 bytes, both
unaligned spans, canaries, overlapping memmove, signaling-NaN payload bits and
unchanged FP flags, empty/negative MOVM, geometry refusal, guarded SELF count and
alias/old-owner retirement. Transfer monitors require the complete independent
byte-by-byte stripe union, and error/destructor monitors require zero guard depth.
Four private source controls omit small/wide claims, payload transfer or restore
old SELF. Mutations are scoped to the diagnostic interval and stop semantic exit 10.
The SELF fixture borrows a REAL rooted startup Prog module under ACTIVE exclusion:
startup native REG.M is nil before the first Prog dispatch. It does not forge a
ModuleLink or claim/execute that queued process. An initial test-only nil-header
fault was corrected and is not counted as VM acceptance.

A separate forced-JIT guest reaches all five actual emitted tails, agrees with
C/default results, unmaps each opcode's live cache independently, exercises real
negative-length error before effects and combined error/unmap, both polls and
NULL/foreign/opcode/held-guard preflight. ABI-aligned CALL and old-emitter gate
controls fail semantic exit 10 before unsafe continuation. 52 selected console/editor
CTests pass; affected tail positives/controls pass again after held-guard preflight.
Native ranges pass TSan and nonrecovering UBSan; actual guarded copy/tail positives
pass TSan. Generated machine bytes remain uninstrumented. No physical ARM/RV or
parallel managed heap acceptance is inferred. SDK/native examples and pins verify.

## Cost and remaining work

Local 20000-loop word/record workload, same binary/image, startup included, warmup
then median of 3: default/guarded C 0.229s/0.263s (~15% extra), default/guarded
JIT 0.158s/0.218s (~38%). Not a whole-IDE, isolated-copy, cross-target or general
statistical claim. This cost is why byte protection is opt-in while local/shared
selection remains absent. No hidden per-copy allocation is used.

Other arithmetic/native writers, mapped ownership/retirement, source/container
lifetimes, root/graph/GC epochs, scheduler/channels and frame/module/code lifetime
remain open. NativeWorld is still required. Dis64 IDs, shared wire, ordinary
WORD32 LEA and default file-call admission are unchanged. Inferno's distinct
numeric-call launch/cache agreement is separate from this memory-access profile.

## Reproduce

```sh
python3 disVM/tests/range_access.py
python3 disVM/runtime/test_shared_bytes.py RUNTIME LIMBO INCLUDE_DIR
python3 disVM/runtime/test_bytes_tail.py RUNTIME LIMBO INCLUDE_DIR
python3 disVM/runtime/benchmark_guarded_copies.py RUNTIME LIMBO INCLUDE_DIR
```

Actual private relinks require matching GNU Makefiles console objects and pinned
source; no production object is replaced. Instrumented positives accept
`--positive-only`. Set TMPDIR if /tmp is unavailable. Installed native test is
`python3 source/tests/range_access.py`; the complete native public/private header
and source closure is installed. The [commented native example](../examples/native-range-access/README.md)
includes exact checkout/SDK commands. See [MOVP](SHARED-POINTER.md),
[canonical object guards](GUARD-REGISTRY.md) and [threading](THREADING.md).

Measured costs and pending investigations: [PERFORMANCE-COSTS.md](PERFORMANCE-COSTS.md).
