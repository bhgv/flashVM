# Dynamic AS bindings and property stores

[Русский](AS_PROPERTIES.ru.md) · [AS2 profile](../../../compiler/as2/view/README.md) ·
[Format](FORMAT.md) · [Grammar](../compiler/fir1.atg) · [AVM1 backend](../compiler/object.c)

Three FIR operations provide the first UI object path without exposing native or
Dis heap layouts. `any` here holds an AVM1 value, including an object reference
owned/rooted by FlashVM. It is not a cross-heap address or an implemented RPC proxy.

| Operation | Inputs/result | Meaning |
| --- | --- | --- |
| `%x: any = as.global_get "name"` | no SSA operands → any | Read the named AVM1 binding. |
| `as.global_set "name", %x` | scalar → void | Assign the named binding. |
| `as.member_set %owner, "key", %x` | any, scalar → void | Set the named member on the captured owner. |

Scalar means i32/f64/bool/string/any, excluding channel and Dis table/cell types.
Text keys follow the existing bounded UTF-8 FIR string-literal contract. The
verifier checks result/operand arity, types, string validity and SSA dominance.
The canonical printer retains keys and operands for reparse/optimization.

All three operations conservatively have every effect bit: memory read/write,
allocation, throw, user call and suspension. The optimizer may not remove, merge
or move a binding read across an unknown store/callback. Backend conversion and
property errors remain ordinary VM errors, never successful dummy values.

AVM1 lowering uses GetVariable, SetVariable and SetMember. Result SSA values use
the existing module-prefixed initializer storage. This remains an initializer
backend, not function-frame/reentrant AS lowering; applications must serialize
view execution on the owning context. SWL admits the emitted SetMember record and
continues checking complete action lengths and branch targets. SWO stays an object;
only SWL creates SWF. No container or Dis opcode is changed.

The Dis backend rejects these operations explicitly until a typed foreign-AS
object module interface exists. An unsupported target does not overwrite a prior
successful output. General object construction, property reads/calls, classes,
method closures and distributed proxies are not provided by these three operations.

The source example `ide/as/editor-view.as` goes through verification, optimizer,
SWO and SWL. The runtime test loads the actual linked SWF, not frontend-authored
bytecode. Separate FIR tests check effect order, canonical roundtrip, invalid
operand types and unsupported-target output retention.

Global names must be nonempty; the backend-reserved `__fir1$` prefix is rejected to protect SSA storage. / Глобальные имена непустые; префикс `__fir1$` зарезервирован для SSA.
