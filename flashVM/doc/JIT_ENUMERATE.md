# Native property-name enumeration — v0.364

[Русское описание](JIT_ENUMERATE.ru.md)

Enumerate and Enumerate2 lower to one native helper with a fixed named/direct
selector. Named enumeration coerces a loaded input, then uses the same qualified
path and ordinary scope services as the interpreter. It deliberately avoids
GetVariable's SWF4 MovieClip result suppression. Lookup retains the physical
selected target independently from script-this and uses the prepared activation's
local-scope policy. Unloaded names must already be strings.

The input/converted name, resolved value and dense key snapshot have separate
GC roots through lookup, snapshot creation and operand-vector reservation.
After success, the input is replaced by the host null or loaded undefined
sentinel, then snapshot keys are pushed in reverse order. Storage is reserved
before publication; the total must fit [FVM_JIT_MAX_STACK](../src/jit/constants.h#L8). A stack-bound failure
returns [FVM_UNSUPPORTED](../include/flashvm/flashvm.h#L46) without replacing the input. Allocation exceptions leave
root chains and active native code restored by the existing execution guards.

| Test | Coverage |
| --- | --- |
| `jit_enumerate` | Isolated host/SWF4–8 contexts in interpreter/manual/automatic modes; named/direct enumeration, exact service snapshot order, hidden inherited names, inherited/accessor/counted-NUL keys, accessor bypass, retained output after code release and GC, snapshot immutability, nonobjects/missing inputs, named native MovieClip including SWF4 |
| `jit_enumerate_callbacks` | Plain target and qualified-path lookup; collecting/throwing name conversion followed by a collecting/throwing getter that deletes itself; detached result retained by the getter capture and then the enumeration roots |
| `jit_enumerate_allocation` | Host/loaded, named/direct allocation failures across invocation, lookup, snapshot and result construction; restored roots/depths/active code; result retained after code release/GC; rejected overflow and exactly full operand vector |

The host accessor fixture explicitly links Object.prototype: host raw objects do
not automatically inherit its addProperty method. Loaded primitive enumeration
keeps the service's no-boxing policy. Enumeration ordering is shared FlashVM
behavior, not an independently verified reference Flash ordering oracle. Function
and exception blocks and other unimplemented Actions still require interpretation;
complete compatibility and real foreign-platform acceptance remain open. No
roadmap stage is closed.

Linux Release: all 741 AGG, 741 Cairo and 722 minimal checks have passing evidence
after generated ELF execute-permission repairs and reruns. Three enumeration
checks pass restricted heap/global ASan+UBSan; stack ASan and LeakSanitizer remain
disabled. The MovieClip test adds a visible custom property so SWF4 suppression
would produce an observably different result.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_JIT_MAX_STACK` | macro / define | [src/jit/constants.h:8](../src/jit/constants.h#L8) |
| `FVM_UNSUPPORTED` | enum constant | [include/flashvm/flashvm.h:46](../include/flashvm/flashvm.h#L46) |
<!-- source-links:end -->
