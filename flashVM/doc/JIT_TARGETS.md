# Native target selection — v0.356

[Русское описание](JIT_TARGETS.ru.md)

SetTarget and SetTarget2 call the shared versioned timeline-selection service.
Literal records require one final terminator and no embedded terminator. Their
paths are copied into immutable code-owned storage and count against the existing
literal limit. Selecting a literal does not push a temporary operand or affect
the runtime operand-stack bound. SetTarget2 retains its input on the rooted stack
through conversion and scene lookup, and pops it only on success.

Selection changes the timeline scope while retaining script-this. Subsequent
native variable/reference operations and named calls use that selected target.
Empty paths reset to the activation's base clip; missing and detached paths retain
the shared host/SWF policy. Absent SetTarget2 operands retain host errors and the
loaded SWF4–6 reset / SWF7–8 invalid-target distinction. Existing activation scopes
restore suspended targets on return, exception and allocation failure.

`jit_targets` compares interpretation, explicit native bodies and automatic native
execution in host/SWF4–8. Cases include literal/dynamic child selection, reset,
missing paths, absent operands, detached aliases, GC during path conversion,
exceptions after selection, and malformed literal records.
`jit_targets_allocation` injects compile and invocation allocation failures for
literal/dynamic paths, collecting coercion and subsequent named dispatch. It checks
restored GC frames, depths, active handles and suspended caller target state.

Ordinary C callbacks continue to block collector reentry under their existing
contract; collection is exercised in the prepared script conversion callback.
Other Actions and all roadmap acceptance gates remain open. These tests do not
provide other-CPU or Windows/macOS/Android runtime acceptance.

Linux Release: all 720 AGG, 720 Cairo and 701 minimal checks pass after restoring
generated ELF execute permissions. The latest two target fixtures were explicitly
rebuilt and pass in all three profiles and restricted heap/global ASan+UBSan.
Stack ASan and LeakSanitizer remain disabled under the existing restrictions.
