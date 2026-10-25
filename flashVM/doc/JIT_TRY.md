# Native Try/Catch/Finally — v0.372

ActionTry now compiles its try, catch and finally blocks into owned native
mappings. It uses the same shared block runner as With: operands, prepared
registers, executed constant pool, source offsets, instruction budget and Return
state stay within the parent invocation. Unsupported child Actions reject the
entire native compilation; automatic execution retains whole-body interpreter
fallback. No AVM1 interpreter is called by the supported native Try path.

The header parser validates flags, three body sizes and the terminated catch name
or register byte. All child mappings belong to the parent handle and participate
in its existing byte/depth limits and active-code protection. An implicit child
ending is uncharged; explicit End remains a source Action. Return propagates
through nested Try and With, with finally able to replace the return or exception.

Before the body runs, a separately rooted operand baseline is captured. A caught
script exception restores that baseline before clearing pending state. A register
catch uses the prepared private or context-global bank, including counted-string
adoption and host/loaded bounds. A loaded named catch uses ordinary selected-target
and lexical assignment; its writes persist. A host named catch temporarily writes
the activation and restores its previous binding afterward. Host root catches
create and publish a temporary managed activation, also used by declarations in
that catch, then remove it before finally and following outer Actions.

Saved thrown values, prior host bindings, Return results and pending exceptions
have roots independent of the live operand stack. Finally clears pending state
while executing; if it ends normally, the prior exception value, pending flags,
rich resource-fault metadata and Return are restored. A finally Return/exception
supersedes the prior result. This follows FlashVM's interpreter: non-script status
failures such as instruction-limit errors skip finally, and a failed catch binding
exits before finally. Callback writes and allocation failures are not transactional.

Root JIT policy selection is now inside its allocation guard. Previously, creating
the empty breakpoint-name key could throw `std::bad_alloc` before the guard, escaping
a root C API call. The allocation regression reproduced that termination; root
entries now return [`FVM_OUT_OF_MEMORY`](../include/flashvm/flashvm.h#L46), with the handled result published correctly.

| Test | Coverage |
| --- | --- |
| `jit_try` | Host/SWF4–8 and three modes; normal/overriding/rethrown exits, explicit/implicit End, restored baseline, host/loaded catch bindings, global/private/ignored registers, counted-NUL strings, pool propagation and nested With/Try |
| `jit_try_callbacks` | GC with objects retained only by the operand baseline, suspended Return, pending exception or old host catch binding; collecting catch/finally; rich fault restoration; finally replacement; active parent/child release rejection |
| `jit_try_allocation` | Compile/invoke allocation sweeps, including cold/warm automatic entries, for named/register catches, host/loaded, manual/automatic function, public root and bounded frame entries; unchanged output handles, cleanup/retry, recursive inactive mappings and exact cache bytes |
| `jit_try_validation` | Malformed headers/bodies, cross-block branches, unsupported children, 64/65-level nesting, root-local catch lifetime, throwing loaded catch setter, exact shared budgets, no finally on budget/catch-binding failure, source offset and cached breakpoint/native reentry |

These are interpreter/native agreement checks, not complete reference Flash
conformance. Audio/quality Actions, remaining standard APIs, full media support
and real Windows/macOS/Android runtime acceptance remain open. No roadmap stage
is closed by this change.

Linux Release: all 767 AGG, 767 Cairo and 748 minimal checks have passing evidence
after ELF permission repairs/reruns. Eight targeted checks pass restricted
heap/global ASan+UBSan; stack ASan and LeakSanitizer are disabled. The expanded
warm-cache allocation sweep passes all profiles and sanitizer. Fresh backend
fixtures pass 26 synthetic Unicorn instruction/ABI cases.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_OUT_OF_MEMORY` | enum constant | [include/flashvm/flashvm.h:46](../include/flashvm/flashvm.h#L46) |
<!-- source-links:end -->
