# Generic rooted Heap-owner holds / runtime0.111

`dvm_runtime_heap_hold(cell,expected,allow_nil,hold)` generalizes the actual list
hold protocol. The production list adapter now uses it: existing DVM_ListHold
layout, Tlist-only/non-H behavior, diagnostics and caller entry points stay.
There is no new module/wire/native-extension ABI or cross-VM pointer channel.

The caller supplies an independently mapped owning slot and genuine live managed
referent. Prepare a one-entry root reservation before cell exclusion. Under the
actual shared cell/range domain, capture the pointer, validate nonzero/non-saturated
count and optional exact Type identity, retain, mark and publish its Heap-header
root. Fully unlock before returning the lease or any static diagnostic. Failure
undoes the extra retain and native reservation; marking is monotone and not rolled
back. No VM error/destructor/allocator/free callback occurs under guards.

`expected=NULL` permits any genuine managed type, not arbitrary native pointers.
Exact type identity does not establish mapping/provenance or legal module lifetime.
A lease keeps identity/lifetime, not payload exclusion after return; later mutable
payload access needs its own guards or an applicable immutable-data contract.
`allow_nil` accepts H as a live active-empty group without fabricating a Heap or
strong count. Stored C NULL still refuses. A live lease refuses overwrite.
One owner exclusively operates the stable lease; no implicit copy or asynchronous
borrow is permitted. The list adapter explicitly consumes/transfers its fields
into synchronous local storage before generic cleanup can unwind.

`dvm_runtime_heap_finish(hold,body_ok)` consumes the held owner outside guards.
body_ok must be an actual completed kprotect result. Lease fields/root reservation
are cleared before actual consuming destroy. A primary body error survives a
secondary retirement error; otherwise the retirement error propagates. A consumed
owner is never retried. Fast can establish its OWN pending-retirement root after
lease withdrawal; zero final roots is checked after actual Fast drainage, not
before the queue has had a chance to consume its owner.

ACTIVE/world admission, frame/referent/Type/GC/code lifetime, collector exclusion
and native destructor contracts remain caller obligations. This primitive is
not a collector snapshot lease, graph publication epoch, allocator lock or safe
handoff of arbitrary aliases. RC1 never proves uniqueness/locality. NativeWorld
still serializes general managed execution. A concurrent constructor additionally
needs destination commit/retry, displaced-owner retirement and graph barriers;
this series does not implement that constructor or remove world serialization.

## Actual evidence

Diagnostics use real String and pointer-List allocations, immediately pinning
construction owners. Generic typed/any-type captures conserve strong references
and actual root counts. Wrong type, stored C NULL, forbidden H, live-output reuse,
both native reservation failures, mark refusal and root-publication refusal leave
no extra owner/root. Allocator/free wrappers reject attempts while object guards
are held. Protected cleanup consumes once and preserves primary/secondary errors.

After all construction pins/original owners are removed, leases alone root the
String/List for16 actual Cycles. Then the direct String lease closes and another
16 Cycles retain it only through the rooted list's payload edge. Payload survives;
final consuming cleanup/Fast drains retirement roots and restores Tstring/Tlist/
Tptr baselines. No fake Heap/native-key root is fed into the collector.

Five PRIVATE actual-source omissions cover retain/mark/publication/exclusion/
unlock, scoped to a designated healthy capture so earlier refusal tests remain
valid. A fixture-only hook selects that interval; no hook is compiled into the
production helper. Header-count observation stops missing exclusion before unsafe
continuation. Exact semantic10/marker alone counts, never a crash/compiler/
sanitizer/resource failure. Surrounding C/JIT guests and both collector polls pass.
The complete hosted example compiles separately; its function bodies execute
normal and throwing callbacks in the actual-object fixture, with only duplicate
native include directives removed. No new JIT lease opcode is introduced. TSan positives instrument actual C paths,
not generated guest machine code or arbitrary concurrent managed mutators.

Existing HEADMP seven semantic controls and pointer/scalar consumers exercise
the actual production adapter. LENL is a separate unaffected regression; it does
not use this owner-hold adapter. This does not prove all owning C/JIT paths,
physical ARM64/RV64, general concurrent graph collection or full IDE behavior.

```sh
python3 disVM/runtime/test_heap_hold.py RUNTIME LIMBO INCLUDE_DIR
python3 disVM/runtime/test_shared_head.py RUNTIME LIMBO INCLUDE_DIR
```

[Commented hosted C callback example](../examples/heap-hold/README.md).
[Cost ledger](PERFORMANCE-COSTS.md) retains HEAPHOLD-111 matched samples. Root
reservation remains two native blocks per capture, not an isolated timing claim.
For the matched benchmark extract the exact old source, never guess a baseline:

```sh
git show 61ceda0ca857c6f02fc8899dbc84cc32196b0ebd:disVM/runtime/list_hold.c > build/list-hold-runtime110.c
python3 disVM/runtime/benchmark_heap_hold.py RUNTIME LIMBO INCLUDE_DIR build/list-hold-runtime110.c
```

The driver checks the baseline SHA before relinking only private list-hold views.
All other matching runtime objects/flags and guarded startup profiles stay equal;
the generic core remains linked but unused on the old side. The installed SDK
includes diagnostic closure, guide, example and raw samples. No baseline/source
checkout is silently downloaded or consulted by a running VM.
