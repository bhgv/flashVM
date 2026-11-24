# Dynamic AS bindings, property stores and method calls

[Русский](AS_PROPERTIES.ru.md) · [AS2 profile](../../../compiler/as2/view/README.md) ·
[Format](FORMAT.md) · [Grammar](../compiler/fir1.atg) · [AVM1 backend](../compiler/object.c)

These FIR operations provide a UI object path without exposing native or
Dis heap layouts. `any` here holds an AVM1 value, including an object reference
owned/rooted by FlashVM. It is not a cross-heap address or an implemented RPC proxy.

| Operation | Inputs/result | Meaning |
| --- | --- | --- |
| `%x: any = as.global_get "name"` | no SSA operands → any | Read the named AVM1 binding. |
| `as.global_set "name", %x` | scalar → void | Assign the named binding. |
| `as.member_set %owner, "key", %x` | any, scalar → void | Set the named member on the captured owner. |
| `%result: any = as.member_call %owner, "key"[, %arg]` | any, optional scalar → any | Invoke the named method on the captured receiver and return its value. |
| `%args: as.args = as.arg %value[, %previous]` | scalar, optional list → list | Extend a compile-time list with one captured argument. |
| `%result: any = as.member_call_args %owner, "key", %args` | any, list → any | Invoke with the captured list in source order. |

Scalar means i32/f64/bool/string/any, excluding channel and Dis table/cell types.
Text keys follow the existing bounded UTF-8 FIR string-literal contract. The
verifier checks result/operand arity, types, string validity and SSA dominance.
The canonical printer retains keys and operands for reparse/optimization.

The argument list takes up to eight values and exists only in FIR. Its
SSA links preserve evaluation order and the target backend loads them in
the AVM1 stack order. It cannot be copied or converted to a script value.
The remaining operations conservatively have every effect bit: memory read/write,
allocation, throw, user call and suspension. The optimizer may not remove, merge
or move a binding read across an unknown store/callback. An unused result still
causes its call to execute. Backend conversion and
property errors remain ordinary VM errors, never successful dummy values.

AVM1 lowering uses GetVariable, SetVariable, SetMember and CallMethod. Result SSA values use
the existing module-prefixed initializer storage. This remains an initializer
backend, not function-frame/reentrant AS lowering; applications must serialize
view execution on the owning context. SWL admits SetMember and CallMethod and
continues checking complete action lengths and branch targets. SWO stays an object;
only SWL creates SWF. No container or Dis opcode is changed.

The Dis backend rejects these operations explicitly until a typed foreign-AS
object module interface exists. An unsupported target does not overwrite a prior
successful output. General object construction, property reads, classes,
method closures and distributed proxies remain outside this path.

The source example `ide/as/editor-view.as` goes through verification, optimizer,
SWO and SWL. The runtime test loads the actual linked SWF, not frontend-authored
bytecode. Separate FIR tests check effect order, canonical roundtrip, invalid
operand types and unsupported-target output retention.

Global names and method keys must be nonempty. The `__fir1$` global prefix
protects SSA storage.
