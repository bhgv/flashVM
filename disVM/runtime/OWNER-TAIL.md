Runtime0.88 update: [the C MOVP target now guards source retain and destination publication](SHARED-POINTER.md).
The reviewed tail ABI/cache/error constraints below remain unchanged.

# Checked x64 owner tail entries — runtime0.84

Hosted SysV x64 LeafOwner lowering for MOVP/SELF/HEADP/TAIL now emits a short tail
JMP into dedicated C entries. It no longer embeds ordinary Heap.ref RMWs or
color/nprop accesses for these four operations. Entries invoke the ACTUAL retained
C opcodes, including their checked Heap acquisition, C Setmark scalar helpers,
alias/packed-slot/nil handling and old-owner destruction. This is a checked C
baseline, not a claim of faster inlined CAS or complete parallel JIT ownership.

The reviewed original x64owner function is hash-pinned before replacement.
Emission requires exact host source/destination/module/list offsets, native Heap
count/color distances/width and actual selector/pending identities/mark color.
No new LeafLayout field, shared native ABI, opcode/section ID or file permission
is introduced. Other architecture emitters are unchanged; ARM64/RV64 and other
x64 allocating/marking families need their own target-specific work.

## Admission and lifetime

Entries refuse NULL, foreign REG, absent/wrong-size context, absent host process,
non-ACTIVE owner or pending progress before operand access. These checks are NOT
memory provenance or a capability to execute arbitrary register contents. Caller
must supply this runtime's actual decoded rooted REG/slots and independently live
source objects under serialized NativeWorld admission. Entry never acquires
admission, strong owners or roots on behalf of an invalid caller.

After entering an opcode, normal completion returns1. VM errors propagate through
normal C checkpoints, never decline/retry after ownership effects. C opcodes
retain their original error semantics; this wrapper does not add transactional
rollback to an error after destination publication/old-owner destruction.

Generated entry keeps REG in SysV rdi and the ORIGINAL C dispatch return address
on stack. MOV RAX/entry plus JMP RAX creates no return PC in the JIT mapping.
C wrapper/helper/opcode code stays process-live. The tail scaffold performs
no JIT/cache access after entering the opcode; C module/frame lifetime
requirements remain those of the retained opcodes. Tests retire only the current
mapping under serialized admission; another thread executing that mapping would
need its own code-lifetime pin or grace-period protocol. Caller still owns
Module/REG/frame/source lifetime and normal error/unwind obligations; retiring
an arbitrary active Module object is not authorized by code-cache retirement.

The existing pointer-cons tail also uses the same context preflight now,
including NULL/unbound-context refusal before operands. The ordinary transactional
constructor API and its source/root/monitor contracts remain unchanged. A fresh
native thread verifies NULL and parent REG decline with no bound VM context.

## Actual checks

`test_owner_tail.py RUNTIME LIMBO INCLUDE` uses matching GNU Makefiles console
objects and actual x64 generated entries. Compiler -F function values force SELF;
real string/list copying/traversal exercises MOVP/HEADP/TAIL. A no-cache-retirement
case checks all FOUR entry counters between guest print markers. Actual C/no-JIT
baseline produces the same32-element result with both polling backends.

Separate cases unmap the CURRENT cache at entry into EACH opcode. A MOVP case
supplies a temporary C nil source and uses actual kprotect to catch its real
pre-publication VM error; destination remains unchanged before a test-only retry.
Another case combines unmap/error. Foreign/NULL REG refusal preserves the actual
REG snapshot. Positives finish normal guest and actual joined GC with pending0.
These do not prove rollback of arbitrary callbacks/destructors or concurrent
managed graph access.

Two private actual-emitter controls either use ABI-aligned CALL (return PC then
belongs to the soon-retirable JIT mapping) or restore the original unchecked
x64owner function. Each requires exact semantic10 before unmap/unsafe cleanup or
at the checked-entry coverage predicate; signals/timeouts/compiler/sanitizer
failures do not count. Private compiler/objects never replace production objects.
`--positive-only` excludes these controls for instrumented builds.

41 selected console/editor CTests pass;3 affected cases pass again after
context-size/host-process and shared pointer-cons preflight changes. Actual owner-tail positives pass TSan with
both poll modes, including per-opcode cache retirement and combined error. Emitted
bytes remain uninstrumented. Existing pointer-list checked-count and tail/error
controls remain separate regression evidence.

## Remaining parallel-heap work

NativeWorld still serializes managed execution. Moving these four paths to C
reuses checked count/scalar marking but does not protect mutable shared payloads,
make RC==1 a reservation, publish roots/edges across collector snapshots or
coordinate GC epoch clearing. Other JIT count/marking families, native modules,
channels/scheduler, graph/root barriers and target ABI/lifetime proofs remain.
Public Dis64 file-call admission remains closed; this ordinary-runtime tail
adapter is not validation or activation of those immutable file-call goldens.
