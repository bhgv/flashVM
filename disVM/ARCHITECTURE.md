# Complementary Dis and SWF execution

[Русский](ARCHITECTURE.ru.md) · [Core boundary](README.md)

DisVM and FlashVM are sibling independent engines. Limbo/Dis modules and AS/SWF
modules must be able to implement and consume shared interfaces. A Limbo `.m`
header is an authoritative module contract for both implementations and callers;
external modules are not limited to native C. AS implementations of non-Dis
modules and AS callers of Dis are first-class directions.

The current own compiler emits descriptor/source scaffolds for those directions.
A real host bridge must still resolve signatures/types/layouts, register external
exports, marshal owned values/capabilities and retain pending GC roots. It must
not bypass typed UI channels, pass raw heap pointers or execute a blocking rendezvous
on the render/event owner. Shared call IDs identify pending calls, not native frames.
Different heaps and schedulers remain distinct. Debuggers must expose boundary
frames and source identities across calls, including suspended/failed calls.

The bridge should use existing native module and channel machinery where appropriate,
without pretending that Dis Runtab/native frames are AS objects. C/AS generation from
`.m` must use a common resolved semantic descriptor; the initial syntactic manifest
is a stepping stone, not a replacement for real runtime signatures.

Later format/runtime unification is possible, but requires a separate design for
module identity, value semantics, GC ownership, instruction semantics, exceptions,
scheduling, debug relocation and link/load compatibility. Do not preempt it with
an undocumented SWF tag or a container that only stores two unrelated binaries.
FIR may lower to either engine while preserving source language semantics.
