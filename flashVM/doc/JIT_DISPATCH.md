# Native URL and stored-frame dispatch — v0.368

[Русское описание](JIT_DISPATCH.ru.md)

GetURL, GetURL2 and Call lower to shared URL/frame services. The VM itself performs
no network or navigation: URL tests register a synchronous recording host sink.
GetURL validates two terminated strings and copies an adjacent immutable literal
pair. Runtime roots adopt both byte strings into the context before delivery;
loaded SWF4–5 rejects non-ASCII literal bytes before adoption, matching the
interpreter. GetURL2 validates method/reserved bits at compile time and retains
both operands through target-first, URL-second conversion and host delivery.
Inputs are consumed only after success; missing loaded inputs use shared padding,
host missing inputs fail, absent sinks remain unsupported, and sink errors propagate.

Call retains the original operand until nested frame dispatch finishes. It uses
the selected timeline/root-recovery rules and shared numeric/label/clip:frame
lookup without seeking the playhead. Stored frame blocks now consult the normal
automatic execution policy after bounded framing validation. Supported blocks
enter JIT with fresh root registers/pool; unsupported blocks still interpret.
The current frame's named breakpoint exclusion is checked before native selection,
so cached code cannot suppress the breakpoint. Caller target, pool, roots, debug
cursor and shared instruction accounting remain scoped by the existing services.

| Test | Coverage |
| --- | --- |
| `jit_dispatch` | Host/SWF4–8, interpreter/manual/automatic modes, all valid URL flag combinations, counted-NUL values, literal/legacy byte policy, host errors/exceptions, absent input/sink and retained strings after code release/GC |
| `jit_frame_dispatch` | Numeric/label/qualified/counted-NUL calls, selected/missing target recovery, unchanged playhead/lower stack, missing frames/inputs, nested exceptions/recovery, bounded recursion and inactive caches after unwind, unsupported frame fallback, named breakpoint/cache reentry, fresh nested constant pool, implicit-End bounded entry, exact shared 7/8-instruction limits |
| `jit_dispatch_callbacks` | Collecting/throwing target-first URL conversion in traced-string GC; converted target retained through URL conversion; original frame request receiver retained through nested frame GC |
| `jit_dispatch_allocation` | Literal/dynamic URL, manual Call and automatic parent/child compilation; compile failures retain existing handles; invoke failures restore roots/depths/active code; host sink allocation failures propagate |
| `jit_invalid` | Incomplete/malformed URL pairs, invalid URL2 flags/width and nonempty Call payload preserve the original code handle |

Native counters distinguish the compiled parent and compiled stored frame from
interpreter/manual fallback; interpreter counters cover named-breakpoint and
unsupported-body fallback. Default execution does not replay effects on errors;
explicit diagnostic replay retains its existing opt-in side-effect policy.
This is existing FlashVM dispatch semantics, not complete browser/FSCommand,
streaming/frame-event or reference Flash compatibility. Real foreign-platform
acceptance and all roadmap stages remain open.

Linux Release: all 754 AGG, 754 Cairo and 735 minimal checks have passing evidence
after generated ELF execute-permission repairs and reruns. Five dispatch/callback/
allocation/frame/invalid checks pass restricted heap/global ASan+UBSan; stack
ASan and LeakSanitizer remain disabled.

### Bounded diagnostic replay — v0.369

Automatic stored-frame entries retain their bounded framing when opt-in exception
replay enters the interpreter. A host frame without an explicit End remains valid
during the retry, just as it was during native entry. Ordinary root entries keep
their existing framing requirements. Retry effects remain observable; the original
exception, instruction budget, roots and active-code state are restored afterward.
`execution_mode_frame_replay` covers host/SWF4–8, explicit/implicit End, replay on/off,
and both successful and throwing retries with exact entry/step counters.

v0.369 validation: 755 AGG / 755 Cairo / 736 minimal passing checks after ELF
permission repairs; four restricted heap/global ASan+UBSan checks pass.
