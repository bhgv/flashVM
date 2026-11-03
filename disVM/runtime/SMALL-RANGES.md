# Compact three-small-span access — runtime0.97

The fifteen guarded ADD/SUB/AND/OR/XOR BYTE/WORD/LONG operations now use
DVM_TripleAccess: six possible boundary stripes instead of DVM_RangeAccess's
256-entry metadata. Both APIs claim the SAME DVM_CellDomain and guard objects.
No payload operation, ordering/ownership check, root policy or admission is removed.
General wide/empty spans keep the existing range API; old access layouts are unchanged.

## API and lifetime

`disvm/small_range.h` declares dvm_small_range_access_begin/end and its new owner-
local metadata type. Each of source, second and destination is 1..sizeof(void*)
bytes. Geometry selects each span's first/last canonical cell. Sort six indices,
deduplicate, retain the domain borrow and try guards in ascending order. BUSY or
refusal releases the acquired reverse prefix and native borrow, clearing metadata.
End requires the exact bound owner and guard depth, then zeroes the access.

Caller keeps the domain and mapped buffers alive independently. No mapping/root/
managed-reference ownership is acquired. Metadata mutexes may wait, object claims
are try-only; do not wait or call a VM/error callback under these guards. Empty/
oversized spans refuse unchanged; use DVM_RangeAccess for those cases. No native
allocation occurs per begin/end. Managed entry preflights exact ACTIVE service
identity before canonical domain lookup. Current arithmetic maps BUSY/refusal to
exHeap before effects; parallel scheduler retry policy remains unfinished.

The native domain still has 256 collision stripes, not a thread/object quota.
Compact capacity 6 concerns one operation's byte boundaries only. NativeWorld still
serializes general managed execution. Other reads/writers, channel transfers,
graph/root/GC epochs and frame/module/code lifetime remain separate requirements.

## Evidence and reproduction

Native mixed tests now alternate two-cell, wide-three and compact-three protocols
on eight joined OS owners: 16000 paired and 10664 disjoint third-counter updates.
Exact six-stripe membership, ordering, alias deduplication, oversize refusal,
cleanup, BUSY rollback and domain retirement are checked. Actual allocation
monitoring covers compact claims. Compact omission controls require exact semantic
exit 10; wide controls remain independent. No fake managed Heap is used.

The unchanged arithmetic oracle still checks 877500 actual C opcode value/alias
cases under real managed-owner admission/kprotect, plus guard refusal/preflight/
error canaries. The real Limbo guest exercises all 15 x64 entries including each
cache unmap, inherited return-PC, error/combined and bad-CALL/old-emitter controls.
Machine code is uninstrumented. These are bounded native/hosted tests, not general
parallel managed/GC or new physical ARM64/RV64 acceptance.

```sh
python3 disVM/tests/small_range.py
python3 disVM/tests/range_access.py
python3 disVM/runtime/test_shared_arithmetic.py RUNTIME LIMBO INCLUDE_DIR
python3 disVM/runtime/test_arithmetic_tail.py RUNTIME LIMBO INCLUDE_DIR
python3 disVM/runtime/benchmark_small_arithmetic.py RUNTIME LIMBO INCLUDE_DIR
```

The last command privately relinks ONLY the arithmetic helper's metadata and
begin/end entry names back to the real protected generic implementation. All
other production objects/flags/opcodes remain identical; both startup profiles
keep --gc-worker --guarded-copies, quantum2000 and one image/checksum. One warmup
then three alternating samples compare generic versus compact for C/JIT. Raw
samples are preserved. This is workload cost, not isolated lock/instruction time.
Private relinking requires matching GNU/CMake console objects. Set TMPDIR where
/tmp is unavailable; --positive-only is supported by correctness tests, not timing.

The SDK installs header/source/test closure, comparison script and the commented
[C example](../examples/native-small-range/README.md). Example native64 result 42
uses unaligned inputs and overlapping output, then the wide API in the same domain.
[Historical costs](PERFORMANCE-COSTS.md) remain retained; reducing one candidate
must not erase prior measurements or imply every overhead is gone. Dis64 wire/
IDs/default and immutable named launch/cache agreement stay unchanged.

## Release evidence and measured cost

All 38 affected checks pass: 37 initial passes plus the isolated owner-role test
rerun after adding small_range.c to its explicit link closure. TSan positives pass
managed arithmetic, all 15 JIT tails and native/forced-lock compact mixing. Installed
native controls/source/header closure and the C example pass. Retained vendor files
verify 1096/312. No new hosted UBSan/physical target or parallel managed acceptance.

Native64 metadata measures 72 bytes versus 2072 for the generic type. In a protected
matched comparison, C generic 3.406701185→compact 2.703036849 seconds (elapsed down
20.66%); JIT 3.421904895→3.031322135 (down 11.41%). Three raw samples per side are
preserved and vary noticeably; these are local workload medians, not universal
speedup or isolated lock cost. The private baseline changes only storage/real guard
entry names and keeps computation, admission, quantum and image/checksum identical.

A separate current full-profile comparison measures default→guarded:
C 2.776729707→2.885887481 seconds (+3.93%); JIT 1.905204034→3.281296924 (+72.23%).
Do not subtract percentages across these two comparisons or historical ARITH-096.
The ledger remains open for remaining overhead; no safety was removed for timing.
