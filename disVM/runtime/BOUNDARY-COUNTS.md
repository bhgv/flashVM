# Loader and exception ownership — runtime0.80

The private hosted Loader view now retains its incoming module-data Heap and
that Heap's Type through the checked native count primitives. The private
exception walker likewise uses checked Heap retention for the emergency String
and for converting an ADT exception to a String while unwinding to its caller.
These four explicit RMW sites previously used plain increments.

`prepare_boundary_counts.py` checks the complete SHA256 of each reviewed source
view after native-register/exception adaptation, then replaces exact single
sites. Repeated adaptation or source drift refuses before output publication.
The retained upstream files and native Heap/Type/wire layouts are unchanged.
Stores, marking barriers and source roots retain their order. Checked refusal
has the existing terminal native invariant policy; it cannot resume a partially
constructed Loader module. This does not add recoverable allocation rollback.

A live source, serialized managed admission, GC marking and correct publication
remain mandatory. Atomic counts do not grant payload access, COW uniqueness,
source lifetime or a root. Emergency-buffer contents, Module/type tables and
exception/process state are still protected by serialized managed admission.
This is not permission for concurrent exception delivery or Loader construction.
Remaining global graph/mark state, payloads, channels/scheduler and target JIT
paths must be coordinated before relaxing NativeWorld exclusion.

## Actual execution checks

`test_boundary_counts.py RUNTIME LIMBO INCLUDE` compiles a normal Limbo program
that constructs data and a one-instruction module through the actual `$Loader`
services. A private Loader source view checks both counts immediately after the
real retention sites, before unsafe cleanup can occur. The instruction is not
executed, and no Dis64 file opcode is enabled. The data map is nil: this test is
about construction ownership, not pointer-map decoding or module execution.

A separate private function appended to the actual exception translation unit
invokes its static `newestring`. A linker interception refuses only the exact
diagnostic message's c2string allocation. The real waserror frame falls back to
the permanent rooted buffer, which must gain a distinct owner. Failure stops
before destroy could consume the permanent owner. Normal managed-owner probes
then continue with real roots/Fast/Cycles and ordinary guest execution.

C/JIT and atomic/locked polling are run. Three private actual-source controls
omit emergency-Heap, Loader-Heap or Loader-Type retention; each must reach the
corresponding semantic10 predicate, never a signal/timeout/compiler failure.
The ADT-to-caller-String increment is hash-adapted but is not covered by its own
missing-retain control in this test. Existing exception boundary tests remain
separate evidence for handler prefixes, nested recovery and terminal budgets.

The script relinks only private exception/Loader objects using matching GNU
Makefiles runtime flags/headers. Production sources/objects are never replaced.
`--positive-only` excludes deliberate controls for instrumented runtime builds.
The source SDK carries adapter/test/contracts; private relinking still requires
a matching native console build and its CMake object metadata, not just an
installed runtime executable.
