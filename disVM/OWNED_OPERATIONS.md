# Owned long-operation locking contract

[Русский](OWNED_OPERATIONS.ru.md) · [Broker](broker/README.md) ·
[Relative sequences](../fir/v1/doc/RELATIVE_SEQUENCES.md)

Status: **native Bigtable primitives now implement per-object guards**.
[Implementation and remaining limits](PAYLOAD_GUARDS.md). Arrays, mutable String
slots and leases spanning source RHS/network waits remain design work.
The current Dis host has one VM owner thread and no application process switch
inside a C array/table copy. The C99 table API explicitly requires serialized
owner access. Broker locks protect diagnostic metadata only.

For shared multi-threaded payloads, a long mutation receives a stable operation
token. The write lock records that token and recursion depth. Nested reads/writes
from the same operation are admitted without unlocking for competitors; another
writer waits. Thread identity alone is insufficient when VM tasks share a thread.
A token must not be forgeable from a remote variable/object ID.

Normalize length/indexes and check the range after acquiring the object guard.
Keep that protection across moves/permutations and final publication. The owner's
internal writes must use the same token; never temporarily open the mutex for
all callers. Readers need a shared guard or a stable snapshot while shape/data
is inconsistent. Copy normally needs a source read guard and destination write
guard; in-place permutation needs exclusive ownership.

Acquire multiple owners in a stable global order, merging duplicate owners before
locking. Recursive entry is not permission to upgrade an arbitrary shared lock.
Release recursion levels exactly once on success, VM exception and allocation
failure. A timeout or disconnected channel is not proof that its executing owner
has stopped: do not unlock or retire its root until completion/cancellation is
acknowledged.

A short spin may protect lock metadata; waiting across scheduler yield, channel,
disk or network must use scheduler-aware queues or native blocking primitives.
Do not hold a spinlock through GC/scheduler callbacks. Long operations may later
be chunked, but retain their operation lease and publish only consistent states.

The broker's independent metadata mutex must never be taken as the payload guard.
Diagnostics/debugger queries can report waiting-for-lock or paused-owner state
without acquiring the target lock. Debug pause does not forcibly release a live
operation's guard. Native table requests now use these tokens and real pthread contention tests.
Array/storage integration and scheduler-aware projection leases remain required.


POSIX implementation / реализация POSIX: [common/owned](../common/owned/README.md).
This implements the guard primitive and native table payload integration.
Реализованы примитив и native-таблицы; остальные объекты VM ещё требуют интеграции.

[Synchronization and implementation status](OWNED_SYNC.md)
