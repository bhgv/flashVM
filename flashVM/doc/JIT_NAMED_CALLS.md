# Native named calls — v0.355

[Русское описание](JIT_NAMED_CALLS.ru.md)

CallFunction and NewObject use shared AVM1 lookup and invocation services.
Loaded name conversion occurs before count conversion. Host calls retain strict
string names; constructors retain their separate argument-count policy. Argument
order, lexical activations, timeline defaults, versioned script-this and qualified
function receivers match the interpreter. Loaded NewObject retains literal name
lookup, so `space.Ctor` can name a global constructor distinct from the `Ctor`
property of `space`; host construction retains qualified lookup.

All source operands stay rooted through conversions, lookup and callbacks. Copied
argument vectors are borrowed by shared services, which establish their own roots.
Results replace a consumed rooted input before stack shrink, without allocating
after shared dispatch. Nested calls use separate invocation state and the existing
native safe boundary; no public ABI changes are required.

| Test | Coverage |
| --- | --- |
| `jit_named_calls` | Host/SWF4–8 interpretation, explicit and automatic native execution; ordered arguments; local function bindings; GC in name/count coercion; throwing/collecting calls and constructors; dotted/colon constructor names |
| `jit_named_calls_allocation` | Host/loaded allocation failures through named lookup, invocation, construction and collecting callbacks; restored roots, depths and active handles |
| `jit_named_scopes` | SWF5–8 timeline receivers; fresh prepared/root activation targets and restoration of suspended caller targets; qualified `owner:method` and `owner.method`; automatic compilation of both caller and callee |

Function definitions, array/object initializers, With/Try and other remaining
Actions still require interpretation. Existing docs for earlier native subsets
record their version-specific scope. All roadmap stages remain open, including
real other-CPU and Windows/macOS/Android runtime acceptance.

Linux Release: all 718 AGG, 718 Cairo and 699 minimal checks pass after restoring
generated ELF execute permissions. All three new checks pass restricted
heap/global ASan+UBSan; stack ASan and LeakSanitizer remain disabled.
