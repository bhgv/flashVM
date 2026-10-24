# AS1/AS2 and Dis module interoperability

Status: required extension design, not an implemented runtime feature. [Русский](MODULE_INTEROP.ru.md).

## Common application model

Limbo remains the primary language of the initial IDE. Parts may be written in AS1/AS2 and collaborate with Limbo modules. A later AS implementation/port of the IDE is allowed; the UI, resource and module contracts must not depend on the implementation language. This is part of FlashVM's superset of SWF <= 8, not a claim that legacy AVM1 natively implements Dis modules.

Both directions are required:

- An AS1/AS2 component can load/resolve a Dis module interface and call its exports.
- An AS1/AS2 component can declare/provide an interface registered as a module, callable from Limbo/Dis.
- AS and Limbo components may exchange explicitly typed channels and data under the same lifetime/serialization contracts.

Module interfaces have an identity, version and checked signatures. Import checks argument/result types and exported names before execution. An AS implementation is exposed by a native Dis-module adapter/proxy; it is not reinterpreted as Dis machine state or bytecode. Keep AVM1 and Dis heaps, stacks, instruction sets and JIT pipelines distinct.

## Calls and scheduling

Begin with correlated asynchronous request/reply invocation. AS1/AS2 do not acquire a synchronous blocking/yielding native call convention merely because a module was loaded. Synchronous-looking language syntax requires a separately implemented continuation/scheduling contract; it must not block FlashVM's owner while waiting for Dis or vice versa.

Calls execute on the callee runtime's owner. A Dis-to-AS adapter queues an AVM1 invocation and resumes the waiting Dis activity when the reply is delivered. AS-to-Dis follows the inverse ownership rule. Define callback/reentrant ordering and avoid a cycle of mutually waiting owners. Requests carry module identity/generation and call identity; load failure, signature mismatch, exception, cancellation, unload and stale result have distinct terminal outcomes.

## Typed boundary

These are required representation rules; the first executable bridge must declare its supported subset and reject the rest explicitly.

| Limbo/Dis value | AS boundary requirement |
| --- | --- |
| byte / int | Checked numeric range/integrality; no silent wrapping or fractional truncation |
| big | Lossless 64-bit wrapper/representation; never silently narrow through binary64 Number |
| real | Explicit IEEE binary64 policy, including nonfinite values and signed zero |
| string | Counted encoding/conversion with documented NUL and SWF-version behavior; no implicit NUL truncation |
| array / list / ADT | An explicitly supported copied graph schema or an opaque typed handle; preserve declared identity/cycles where promised |
| channel | Typed endpoint handle with lifetime, flow control and cancel/close semantics |
| module / callable | Checked module interface and exported operation, not an arbitrary C function name or address |

No native structs, raw pointers or GC heap addresses cross as application payloads. An opaque handle does not make a foreign object a locally owned AVM1/Dis object. Pending calls retain each runtime's own roots until success, failure or cancellation; unload and disconnect release or retire them explicitly. Cross-collector cycles need a designed handle/lease policy, not an assumption that either GC sees the other heap.

## UI remains channel-only

A module call is not an escape hatch around the UI protocol. UI controls and widgets are still manipulated only through their channel service, whether their calling logic is Limbo or AS. Complex AS components may be module implementations, but receive endpoint capabilities and messages instead of unrestricted foreign MovieClip pointers. Native owner-side handlers may call the relevant FlashVM implementation APIs.

## JIT, debugging and acceptance

The module boundary must work with interpreted/JIT callers and callees. Required debugger behavior includes language/module-tagged boundary frames, pending-call state, source/Dis-PC/native mappings where available, exceptions and stepping across the boundary without losing the asynchronous call identity. A module reload retires old signatures, breakpoints, references and pending completions according to an explicit policy.

Acceptance needs real AS-to-Dis and Dis-to-AS calls, typed input/output rejection, exact big/string cases, channels, errors/cancellation, GC while a call is pending, unload/reload, nested calls without deadlock, and interpreter/JIT combinations. A composed Limbo/AS IDE control must still update through channels and receive real input events. These requirements are not covered by the existing numeric FIR or host Python build tests.

[Host architecture](ARCHITECTURE.md) · [Integration sequence](DIS_ACCEPTANCE.md) · [Readable code rules](AGENTS.md).
