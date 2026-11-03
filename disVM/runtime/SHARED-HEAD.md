# Rooted guarded HEADMP — runtime0.92

`--gc-worker --guarded-copies` now selects guarded `HEADMP`, alongside `MOVMP`
and the five scalar/raw copies. Default execution retains the original HEADMP
body. Hosted x64 enters the same actual C operation through its existing aggregate
tail. The guarded profile still refuses other JIT backends; explicit C target
acceptance remains separate. NativeWorld still serializes general managed code.

## Why the list itself needs ownership

Guarding payload bytes does not keep their containing list allocation alive. The
destination may replace the very slot that owned the source list. Destruction of
old output can also invoke callbacks, remove other owners, change REG or run GC.
The new `list_hold.c` algorithm therefore captures a complete independent List
owner under the source-slot guard and roots it through all fallible work. The
`shared_head.c` module validates/project its payload and composes the existing
mapped transaction; opcode generation stays in `prepare_shared_head.py`.

1. Check exact ACTIVE worker role and source-slot geometry. Prepare one native
   root-group reservation before guards (two allocations). Claim the source slot
   through the same cell/range domain as MOVP/raw access, then load its List.
2. Check nonnil value, native geometry, live nonsaturated count and Tlist identity.
   Retain, mark and publish one actual Heap-header root before unlock. On refusal,
   undo only the new count, unlock, dispose metadata and return a static exception.
   Conservative marking already performed is not rolled back. Output hold remains
   unchanged. These checks do not prove arbitrary-pointer/allocator provenance;
   caller independently owns the actual mapped cell and its existing referent.
3. With the list rooted, call the retained module/index/Type validators in their
   original order. The resolver is trusted process-live C, not a script callback.
   Empty record extraction does not access destination or payload storage.
4. Check destination/span arithmetic, actual `poolmsize` extent and compatible
   pointer maps before payload references. A typed list record must have the same
   size and equivalent map; omitted trailing zero map bytes are equivalent. A raw
   list has an all-zero map and must physically contain the requested payload.
   A count/root or numeric address alone is not the allocation proof: the actual
   independently retained Tlist allocation and admitted arena remain required.
5. Preserve historical REG.s projection, then call the existing guarded mapped
   replacement. Its snapshot/retains, rooted old-output retirement and callback
   error draining remain governed by [SHARED-MAPPED.md](SHARED-MAPPED.md).
6. After normal/error return, clear and free native list-root metadata, consume
   the temporary strong owner exactly once and finish. Metadata is cleared before
   the consuming destructor can raise. A body error survives a later release error;
   otherwise a release error propagates. No completed copy/consumed owner is retried.

The source List's stored descriptor remains owned by that node. Requested module/
Type lifetime still relies on existing VM admission; the temporary List owner
does not solve arbitrary module/descriptor races. The hold record is stable,
zeroed, private and disjoint from the borrowed source cell, with one native user.
No OS cancellation or arbitrary native unwinding is introduced.

Source/metadata nil order and normal empty behavior remain visible. This opt-in
path adds native preparation failures before capture and additional count/type/
extent/map refusals for malformed data. It does not silently fall back or change
default admission/error behavior. No VM/error/destructor callback or native wait
spans object guards. Native metadata mutex acquisition may wait, as in the
existing root/cell APIs; allocation-free publication is not wait-free execution.

## Actual evidence

An allocator/free ledger follows the actual root-reservation blocks through every
normal/refusal/error path and requires a zero baseline. It excludes unrelated VM
storage and the ordinary registry's intentionally process-lifetime reusable nodes.
The test-only ledger capacity is not a production object/thread limit.


The dedicated test builds real lists with retained cons, dtype, incmem and native
pool APIs; no Heap/Module header is fabricated. Native record/list slots are
deliberately unaligned. A separate compiled guest exercises the original module/
index resolver, while the diagnostic's explicitly trusted resolver supplies real
dtype results to isolate container/error behavior.

Checks cover invalid-mark rollback, all seven preparation refusals (two list-root
plus five mapped-copy allocations), descriptor and consuming-release errors,
actual allocated extent, incompatible maps, empty destination avoidance, nil-head
precedence, raw payload and zero-tail map compatibility. The destination may alias
the source list slot. After publication, a consuming old-output callback removes
the original list owner/root and performs 32 genuine native Cycles passes: the
temporary hold is then its ONLY strong owner/root. Payload bytes survive. The
callback changes REG.s/d/m and raises after consuming its owner; a second error
after list release must not replace that first error. Final real Fast restores
List/String/descriptor baselines. It also tests otherwise successful empty work
followed by a release error.

Seven actual-boundary controls omit cell acquisition, the new retain, list-root
publication, unlock, temporary-owner consumption, rollback or native metadata disposal. Each must reach
semantic exit 10 BEFORE unsafe ownership/GC continuation. Signals, compiler errors,
sanitizer failures and timeouts are not accepted as controls. C/JIT and both polling
backends run them. Guarded aggregate-tail tests also exercise cache retirement,
invalid-Type errors, combined cache/error, NULL/foreign/held-guard refusal and
bad-CALL/old-emitter controls. Both guarded MOVMP and HEADMP tails refuse existing
guards before operands. Other tails/default selection remain unchanged.

The original 22 affected console/editor regression tests pass. After extending
map compatibility to implicit zero tails, affected head/tail checks repeat with
the final implementation. Actual head/guarded-tail positives also run under TSan;
emitted machine bytes remain uninstrumented. No physical ARM64/RV64 or parallel
managed mutation acceptance is implied. Final head/tail checks and TSan positives pass with zero-tail compatibility.
Checkout/installed examples, installed root suites and both retained vendor pins
verify. No new UBSan instrumentation of the hosted head path is claimed.

Local same-image 20000-iteration list-head workload, startup included, warmup then
median of three alternating samples: C default/guarded 0.106s/0.144s (~36% extra),
JIT 0.099s/0.152s (~54% extra). It includes the whole guarded profile and other
instructions, not isolated HEADMP, whole-IDE, cross-target or general statistical
evidence. Preparation/locking currently cost time; this is why the path is opt-in.

## Reproduce and remaining work

```sh
python3 disVM/runtime/test_shared_head.py RUNTIME LIMBO INCLUDE_DIR
python3 disVM/runtime/test_aggregate_tail.py RUNTIME LIMBO INCLUDE_DIR --guarded-copies
python3 disVM/runtime/benchmark_shared_head.py RUNTIME LIMBO INCLUDE_DIR
```

Private hosted relinks require matching console CMake/GNU-linker objects; positive
instrumented runs accept `--positive-only`. Set TMPDIR when /tmp is unavailable.
The full native source/header closure is installed under `source/threading/`.
The [commented Limbo example](../examples/guarded-list-head/README.md) runs ordinary
C, guarded C and x64 JIT with the same image.

Mapped HEADMP now prepares up to seven native blocks per call. It is an opt-in
safety baseline, not a JIT acceleration or compiler local/shared proof. Reuse and
local selection remain optimization work. Other list/reference/arithmetic writers,
channel/scheduler operations, shared graph/root/GC epochs and frame/module/code
lifetimes still prevent removing NativeWorld. No Dis64 wire/encoding/feature/ABI
ID, ordinary WORD32 LEA, default file-call admission or Inferno named launch/cache
agreement is changed by these native helpers. Retained upstream stays pinned.
