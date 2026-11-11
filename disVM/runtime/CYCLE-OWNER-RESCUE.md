# Shared cycle-owner publication and rescue

This follow-up closes a reference-publication gap after the
[physical Fast finalization series](FAST-FINALIZATION.md). It does not switch the
main runtime to the closed concurrent backend or remove all stopped tracing.

## Shared owners are cycle seeds too

`dvm_runtime_rc_defer` formerly recognized a cycle candidate only from ACTIVE
admission. A SHARED service releasing a native ticket could therefore atomically
drop its nonfinal record reference without creating a candidate. A closed graph
then depended on the general retained tracer to discover that lost outside owner.

Candidate recognition now applies to both admitted producer roles. A supported
nonfinal record/list/array/channel reference becomes an indexed Cycles owner or
stays strongly owned and rooted in the mandatory FIFO if optional publication
cannot proceed. The release does not invent a new reference: it transfers the
existing one and adds its tracing root before the source ticket removes its root.
`--gc-no-cycles` retains its diagnostic meaning; it is not used by this acceptance.

## Mandatory FIFO to Cycles

[owner_rescue.inc](gc/cycles/owner_rescue.inc) runs on the sole shared Fast consumer.
It captures the live allocation generation and reserves a Cycles slot before
withdrawing the mandatory head. The existing root occurrence remains unchanged
through source withdrawal and destination commit. Publication serializes with
other candidate-index producers using the existing `candidate_transfer` mutex.
Allocation and worker notification remain outside that mutex. No managed payload
is read after publication, because the receiving collector may immediately use
or consume the transferred reference.

| Result | Ownership | Fast dispatch |
| --- | --- | --- |
| Successful handoff | One strong owner and its original root move to Cycles | Count completed work |
| Transient slot/byte pressure or an indexed duplicate | Mandatory entry, reference and root remain intact | Request a paced shared retry |
| Closed/unavailable service, invalid generation or nonretryable reservation status | Mandatory entry, reference and root remain intact | Preserve the existing retained path |

The mandatory drain still charges each visited head against its slice and rotates
unchanged refused heads. Closed queues must not trigger an endless request for a
publication that shutdown has forbidden. That distinction preserves the remaining
mixed-runtime shutdown protocol; it does not make shutdown stop-free.

`NATIVEMANDATORYCYCLE transferred=N retries=M` counts actual root-preserving
handoffs and duplicate-index observations. Internal transfers do not increment
the accepted reference count again; joined RC diagnostics must remain balanced.

## Why an indexed duplicate is retried

An indexed candidate is not a separately acquired lifetime claim. Cycles may
already have decided to drop its queue reference because the mandatory owner
still contributes another external root. Dropping the mandatory owner merely
because the key is still indexed could remove both queue owners and leave a
cycle without a candidate.

Fast therefore keeps its reference and root until the prior index is withdrawn,
then offers the mandatory owner as another seed. Atomic nonfinal refcount changes
alone cannot prove that a concurrent candidate decision will rediscover the
remaining cycle. This protocol does not add a global graph or execution mutex.

## Actual heap acceptance

[test_cycle_owner_rescue.py](test_cycle_owner_rescue.py) uses real allocated record
descriptors, mapped fields, reference counts, managed tickets, queues and collector
objects. Two permanent peers alternate admission epochs so at least one always
remains active. Every target `poolfree` must occur with both peers admitted,
without stopping/stopped world state, with no remaining root for that allocation,
and exactly once. Allocation generation keys distinguish a target from later
address reuse. Only after the actual allocator returns is completion counted.
The independent Type creator owner must remain alive and return to one reference
when every target record has physically gone.

Four positive scenarios run C/JIT and both poll backends:

- a two-node cycle published directly by the shared ticket service;
- an injected candidate-slot refusal followed by mandatory FIFO rescue;
- two independently rooted tickets for the same seed, with a forced indexed
  duplicate overlap and several actual Fast responses before Cycles resumes;
- a 2300-node ring exceeding bounded Cycles and the initial Duke capture capacity.
  This must report resumed Duke slices, adopted discovery pins and a promoted
  snapshot, and must physically free all 2300 allocations before peers leave.

The barrier delays Cycles consumption only until the required publication race
is established; it then calls the unchanged production algorithm. Fast progress
is never fabricated by the observer. World API observers reject stopped Fast
admission while rescue is pending. Cycles retains the mixed backend policy, so
these checks do not forbid every attempted Cycles fallback.

Four private semantic omission controls restore the old shared-reference drop,
disable rescue, remove the original root before destination commit, or omit the
pending-duplicate retry. Each must fail with the exact predicate and status 12.
The duplicate barrier was strengthened after an early control could pass by
opening Cycles before Fast tried its missing retry. Compilation failures,
timeouts and sanitizer warnings are never accepted as control detection.

## Scope and remaining work

The new tests exercise native-ticket cycle seeds, ordinary record rings, direct
and mandatory publication, duplicate index handoff and large shared snapshots.
They do not establish collection of every runtime graph shape, every live-field
conflict, arbitrary native destructors, shutdown without a stop, or memory recovery
under persistent exhaustion. Unknown finalizers and the retained general root
tracer remain. Closed/error-result distinctions are reviewed against the existing
shutdown protocol; no new claim of complete shutdown fault-injection coverage is
made by this series.

Validation is local Linux x86-64. Generated JIT instructions are not instrumented
by C ThreadSanitizer. ARM64/RV64 execution and full runtime race-freedom remain
unverified. See the [current roadmap](MULTITHREADING-ROADMAP.ru.md) and prior
acceptance investigations in [CONCURRENT-GC.md](CONCURRENT-GC.md).

## Validation

The final normal selections passed 77 ownership/root/resident/native tests and
10 additional GC tests, including native/idle cycle GC and existing large graph
workloads. Ten fully instrumented runtime repetitions passed all sixteen positive
profiles above: 160 application executions, with 92,000 large-ring record frees
and 240 small-cycle record frees observed before peer release. This evidence
covers the exercised native protocols; it does not make JIT-generated instructions
TSan-instrumented or turn the mixed backend into a stop-free implementation.

The final generation-aware allocation observer was then rerun with all four
semantic omission controls and with all sixteen instrumented positive profiles;
both runs passed. Earlier acceptance investigations, including the intermittent
receiver-cancellation observation, remain documented in the prior reports.
