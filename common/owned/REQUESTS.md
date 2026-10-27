# Guarded scalar requests

[Русский](REQUESTS.ru.md) · [Owner contract](README.md) ·
[Native VM integration](../../disVM/PAYLOAD_GUARDS.md)

`table_request.h` is the shared implementation used by hosted DisVM Bigtable
operations and available to trusted host adapters. It is a local C API, not an
RPC wire record: borrowed pointers must never cross heaps/processes or the network.
All users retain externally valid owner lifetimes and disable native cancellation
through request execution and result adoption/destruction.

`fvm_owned_token` issues process-local nonzero operation tokens under a short
metadata mutex. Tokens never wrap/reuse; exhaustion returns EOVERFLOW. Remote
input cannot select a trusted token. Sequential nested helpers use the same token;
two simultaneous native continuations must never share it.

`fvm_table_request` returns native guard errors separately from `request.status`.
GET, SET, DELETE, LENGTH, COUNT, RESOLVE, RANGE, SLICE and KEYS guard the complete
operation. GET reads a scalar; RANGE normalizes bounds under that same guard.
SLICE/KEYS transfer independently owned snapshots only on success. SPLICE guards
source and destination, including all source reads, allocation, tail replacement
and final publication. The configured C allocator must return normally, without
VM exceptions or longjmp through a live guard.

Pairs are acquired in stable native address order; duplicate owners collapse to
one recursive entry. Failure unwinds only levels acquired by this pair call.
Partially holding the high guard while requesting a free low guard returns
EDEADLK. A caller already holding unrelated guards must obey the same global
ordering; this is not an arbitrary lock-upgrade facility. Leave prevalidates both
owners, so a wrong token does not partially release a pair.

The tests perform 8000 copies in opposing directions on four actual pthreads,
check recursive pairs, wrong tokens, inversion refusal, self-copy rollback under
allocation failure, and quiescent owner retirement. Other tests exercise metadata
inspection while four contenders are parked and 40000 nested table updates.
Allocator accounting checks outstanding owned blocks independently of LSan.

The native VM validates its private GC-managed handle, disables cancellation,
allocates a trusted token, executes this request layer, releases every guard and
only then calls the VM error mechanism. Immutable key snapshots do not need a
mutable payload guard after construction. The standalone mutex may block a
native thread; do not hold it across a yield on the sole VM owner if the awaited
continuation itself needs that owner. Full expression leases, network cancellation
acknowledgments and scheduler-aware remote projections remain separate work.
