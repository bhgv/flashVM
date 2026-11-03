# Transactional pointer-list constructor — runtime0.76

Hosted `consp` now calls `dvm_runtime_pointer_cons(source_slot, destination_slot)`.
The builder captures source and original tail before allocation, including
aliased slots. It checks native Heap and signed Type retains at the actual
acquisition, records each acquired owner, then allocates an unpublished node.
The complete staged node owns its source/Tptr/Tlist and has an explicit root
before the allocation monitor can inspect it. Tail remains privately retained
until final publication. No partially initialized list is exposed in destination.

On success it installs the held tail and publishes the node, removes its staging
root and consumes the former destination owner by a checked nonfinal release.
The node's distinct tail owner guarantees LIVE, avoiding a destructor or deferred
RC queue operation for that replaced owner. Nil source/tail use H;
native null refuses. A non-list destination referent refuses before acquisition.
Geometry checks do not establish host-memory provenance.

On VM error, roots and acquired owners are rolled back before rethrow. Before
node allocation, this uses checked nonfinal releases. After complete staging,
the private count must reach LAST; source/descriptors are released and private
pool storage is freed without a managed destructor or monitor callback. This is
necessary when the monitor itself raised: invoking it again from destroy during
rollback would introduce a second error boundary. The unpublished node must
never escape or acquire external owners. No normal published object uses this
private rollback path. Marking can conservatively remain after a failed build.

Caller must hold serialized managed admission and keep source/tail slots and
complete VM or explicit C roots stable across allocation/monitor callbacks.
A monitor may inspect or raise, but cannot mutate slots/node ownership, retain
or publish the borrowed node, or leave admission. This is not a sandbox for an
arbitrary callback or permission for concurrent payload mutation. No lock,
reference count or address check alone supplies those lifetime guarantees.

The real runtime probe covers source and descriptor counts, complete monitor
view, real VM error and rollback, source/destination aliasing, nil construction,
non-list refusal and real Fast retirement to baseline. Private controls remove
one descriptor retain or rollback; others refuse Type acquisition/allocation and
verify complete rollback before a fresh test-only retry. Inline GC separately
checks monitor failure without recursive destruction callbacks. Compile, signal,
timeout or sanitizer failures never count as semantic detection.

`test_pointer_builder.py` uses matching native Makefiles/GNU link metadata and
private wrappers; production objects remain unchanged. The existing mixed-JIT
list test now observes successful primitive Type acquisitions, including this
builder. Direct pointer-cons machine lowering remains declined. No LeafLayout
ABI, wire/opcode ID or shared Dis64 representation changes are introduced.
Pinned upstream stays untouched; source adaptation refuses a changed opcode hash.
This C boundary is a prerequisite for a future audited native-call lowering,
not that lowering or a completed parallel allocator/GC protocol.


## Runtime0.77: audited SysV x64 tail entry

The checked constructor now has a generated x64 tail entry; it no longer needs
the generic pointer-op fallback on that target. ARM64/RV64 keep their C paths.
The direct unchecked machine-count implementation remains disabled. See
[tail ABI, code lifetime and error proofs](POINTER-TAIL.md). Constructor roots,
slot ownership and serialized admission requirements remain unchanged.
