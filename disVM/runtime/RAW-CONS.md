# Guarded unmapped-record CONSM — runtime0.117

`raw_cons.c` now serves actual selected hosted CONSM and its existing aggregate
SysV x64 tail entry. It handles **unmapped bytes only**: references needing traced
maps/ownership belong to CONSMP, which is not switched. Independently live mapped
owning operands, rooted frames/containers, stable Type/frame/code/cell lifetime,
ACTIVE/NativeWorld, ordinary allocator/GC exclusion and no object guards remain
caller preconditions. Geometry is not mapping/provenance. This is not general
parallel mutation or a new instruction/serialized ABI.

## Capture, publication and cleanup

1. Capture native signed WORD size under its exact guard. Reject negative or
   oversized counts and invalid source/destination geometry before allocation.
2. For nonzero count allocate one private native snapshot, then copy the source
   under span/range exclusion. Zero ignores source and allocates no snapshot.
   These are separate count/source captures: NativeWorld supplies the present
   exclusion. They are NOT a coherent transaction against future parallel writers.
3. Capture current Tlist/H destination, prepare a complete rooted raw-head node,
   and conditionally publish it using the existing Heap commit protocol.
4. On stale tail discard candidate/observation, capture fresh tail and rebuild,
   at most eight preparations. The same immutable source/count snapshot is used:
   callbacks may change original operands via compliant writers without changing
   the operation's captured payload. They may not retain private candidates/holds
   or leave admission. Intervening destination writes are never blindly overwritten.
5. Drain all candidate/retired/tail leases, each under protected consuming cleanup.
   Preserve the primary body error, otherwise the first cleanup error. Free the
   native snapshot after the entire drain and before rethrowing. Never retry a
   consumed owner. A cleanup error after commit does not undo publication.

Deferred candidate retirement may still be rooted in the Fast queue; closing a
lease is not immediate recursive reclamation. All allocating/error/destructor/
monitor work stays outside object guards. Unselected/serial-inline mode declines
before operands to the original C body. Selected x64 already tail-enters CONSM's
C operation; its guard preflight now rejects held guards before decoded reads.
No RX return PC or borrowed REG is read after callbacks may retire current code.

## Runnable example and evidence

`test_raw_cons.c` is the complete commented native fixture. Eight transaction
cases cover ordinary/H-tail/alias, one and repeated stale writes, alias+stale
snapshot preservation, primary/secondary cleanup errors and first cleanup error
after successful publication. Seven refusals cover four root reservation blocks,
node allocation, Tlist retain and native snapshot allocation. Snapshot allocation/
free is tracked exactly, including error drain and repeated stale attempts.

Five additional extents0/1/17/257/513 use actual range guards. The first callback
changes original bytes AND count and publishes a fresh tail: the second attempt
must still publish the original complete snapshot with the fresh tail. Negative/
oversized counts, nonempty NULL source and NULL destination refuse. Real mapped
containers, Strings, binary64 bit identities, Cycles/Fast and Type baselines are
used; no fabricated Heap/Type/scheduler root. A Limbo pointer-free Pair list drives
actual CONSM in C/JIT, both collector polls. Four scoped source controls detect
missing stale cleanup, incomplete unwind, snapshot leak and primary-error loss,
with exact semantic10; crashes/compiler/sanitizer/resource failures do not count.

```
python3 disVM/runtime/test_raw_cons.py \
  build/runtime/disvm build/install1804/disvm-runtime/limbo-native disVM/upstream/module
python3 disVM/runtime/test_raw_tail.py \
  build/runtime/disvm build/install1804/disvm-runtime/limbo-native disVM/upstream/module
```

The actual raw x64 tail fixture checks foreign/cold REG, current code unmap,
VM error, combined unmap/error and strict bad-CALL detection. Native-C TSan is
separate from emitted-code evidence. Installed source closure is checked; no
physical ARM64/RV64/Android/full IDE or concurrent collector acceptance follows.
See [cost paths](PERFORMANCE-COSTS.md), [preparation](VALUE-PREPARE.md) and
[conditional commit](HEAP-COMMIT.md).
