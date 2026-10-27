# Native table payload ownership and exception recovery

[Русский](PAYLOAD_GUARDS.ru.md) · [Local request API](../common/owned/REQUESTS.md) ·
[Executable Limbo example](examples/table-guards/main.b)

Hosted Bigtable's private nonmoving GC owner now embeds `FvmTableOwner`, including
its operation-token recursive guard. All mutable table reads/writes, length/count,
relative key/range capture, slicing and key snapshot construction use guarded
requests. A splice locks both source and destination in stable address order;
self-splice locks one owner. Aliased handles reference the same payload guard.
Private construction adopts a complete snapshot before returning to any program.

Native request execution uses ordinary scalar C storage allocation and returns
Fht statuses. It never calls VM error/GC/application code while a guard is held.
Validation and String/snapshot-key conversion happen before entry; VM exceptions
are raised after leave. The frame retains live heap handles throughout. GC retires
a guard only after its owner is quiescent, then frees scalar storage. Retiring an
active native owner is an invariant violation, not permission to unlock work.

`Bigtable.guardstate(handle)` returns `(owner, depth, waiters)` using only the
short metadata mutex. It does not acquire the payload guard, expose a native
pointer, accept a trusted operation token, or emit successful-operation traffic.
This can support diagnostic adapters; it is not a complete remote broker binding.

The runtime now connects the retained Inferno exception walker to its process
scheduler. Handled Limbo `exception` blocks resume in interpreter and leaf-JIT
modes; caller/native frames are unwound with their exact root maps. A bounded
fallback error String is registered through the existing native GC root table
for the runtime lifetime. Allocator-failure fallback is retained; native thread
cancellation is not an application exception mechanism. Last-exception diagnostic
storage and typed exception values are released on normal scheduler process retirement.
Instruction-boundary budget failures remain terminal, so a handler cannot bypass
execution limits or pretend a not-yet-fetched instruction has executed.

A shared Inferno fix validates predecessor PCs before handler selection, including
builtin modules without bytecode. The parent incorrectly accepted a PC inside
an instruction. A second LP64 fix matches the absent-handler sentinel to the
loader's native ulong width instead of selecting an invalid default target. Both emu/os walkers and native/WORD32 fixture models pass the
regression. Sources remain byte-exact; only TLS registers, root registration and
host diagnostic services are adapted in the generated runtime view.

These guards cover individual table primitives. They do not yet cover a whole
compound source expression across RHS evaluation, network/channel waits, arrays,
mutable VM String slots, or general remote projection leases. Strings currently
use independent value replacements. The host still has one VM executor thread;
trusted native users may exercise scalar owner requests without entering its GC
heap, but raw GC owners must never be passed to a foreign executor thread. A
scheduler-aware queue/pinning/cancellation protocol is needed before async work
can hold these VM owners across requests. The original broker metadata lock stays
independent and is not substituted for the payload guard.


## Retained cooperative ownership

See the [concurrent Limbo tutorial](examples/table-leases/README.md) for explicit
`Bigtable.acquire/release` leases across channel waits, exception cleanup and
the exact local-only scope. Individual primitive guards remain the default.
