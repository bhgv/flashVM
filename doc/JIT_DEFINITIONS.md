# Native function definitions — v0.370

DefineFunction and DefineFunction2 now compile into a direct, exception-contained
function-definition helper. The parent body stays native when these Actions are
supported; each created function independently follows normal interpreter/manual/
automatic execution selection when called. A function body may still require
interpreter fallback even though its defining parent compiles.

The compiler validates record boundaries, reserved Function2 flags and nested
body framing. It accepts both the standard out-of-line body and the historical
inline representation already accepted by the interpreter. Function body bytes
are skipped as one defining source Action: branches into those bytes are invalid.
The immutable code handle owns the encoded definition, without managed references.

Interpreter and JIT now share parameter/name decoding, captured-pool copying and
body framing normalization. Creation snapshots the pool selected when the Action
executes, not the last pool appearing in source. A subsequently selected pool or
released parent code handle cannot change that snapshot. The resulting managed
function retains its lexical environment, parameter/register metadata and body.
Function2 preloads and parameter initialization remain in the shared activation
builder. Context-free compilation accepts loaded register rules; a manually
attached native body still checks strict host record bounds at execution.

Named definitions use the existing selected timeline/global destination, lexical
scope and prepared activation declaration service. The legacy named-body registry
is updated before the declaration, matching the interpreter; a throwing setter
does not roll back earlier effects. The new function and counted name remain
rooted through declaration callbacks. Anonymous definitions publish a function on
the rooted operand vector, respecting its existing native capacity bound.

| Test | Evidence |
| --- | --- |
| `jit_definitions` | Host/SWF4–8, interpreter/manual/automatic entries, both record forms, named/anonymous definitions, explicit/implicit body End, parameters/private registers, global preload, nested definitions, lexical/pool capture after parent release and traced-string GC |
| `jit_definitions_validation` | Malformed headers/bodies/flags preserve the previous output handle; branches into nested body bytes are rejected; host versus loaded register/preload bounds; anonymous stack capacity; unsupported child falls back independently |
| `jit_definitions_callbacks` | Selected MovieClip declaration invokes a collecting script setter; newly created function is callable inside the setter and after GC; original setter exception and all roots/depths/pool state are preserved |
| `jit_definitions_allocation` | Every compile/invoke allocation boundary for both Actions, named/anonymous and host/loaded contexts, manual/automatic parent entries; unchanged code handles on failure, restored roots/active code and a callable successful result |

Existing mode/frame fallback fixtures now use an unreachable ToggleQuality Action,
which remains unsupported by the native compiler. With and Try are still native
compiler gaps. These checks establish agreement with FlashVM's interpreter, not
complete reference Flash conformance or acceptance on other operating systems.
All roadmap stages remain open.

Linux Release: all 759 AGG, 759 Cairo and 740 minimal checks have passing evidence
after generated ELF permission repairs/reruns. Five targeted checks pass restricted
heap/global ASan+UBSan with stack ASan/LeakSanitizer disabled. Fresh backend fixtures
pass 26 synthetic Unicorn cases; these do not execute the complete foreign VM.

Update v0.371: [native With](JIT_WITH.md) closes the earlier With compiler gap;
Try and complete conformance remain open.
