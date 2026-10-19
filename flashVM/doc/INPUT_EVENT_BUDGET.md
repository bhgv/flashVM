# Input event execution budgets

Since v0.618, Key, Mouse and Selection registration, listener inspection and event
delivery share `FVM_Config.instruction_limit` with nested AVM1 and AsBroadcaster.
An entire synchronous event has one budget instead of a fresh budget per listener.
This applies on the VM owner thread through C APIs, extracted methods,
interpreter and supported manual/automatic JIT. [Русский](INPUT_EVENT_BUDGET.ru.md).

## Charged work

| Operation | Native work units |
| --- | --- |
| Add listener | One per examined identity; for a new identity, one per existing entry for possible vector relocation plus one for insertion. Relocation is charged even with spare capacity, independently of allocation history. |
| Remove listener | One per examined identity and one per following entry shifted after a match. Missing removal also charges its search. |
| `_listeners` inspection | One per copied listener in the detached Array. Construction may allocate before the first charge; failure exposes no partial result and changes no registrations. |
| Key dispatch | One per captured receiver, including the focused clip; one before state publication; one before each optional handler lookup/call. |
| Mouse down/move/up/wheel | One per captured listener; one before pointer publication; one before each optional handler lookup/call. Drag updates after publication, before delivery. |
| Selection.setFocus | One per captured observer; one before focus publication; one before each old/new clip endpoint and observer lookup/call. Same-focus requests cause no transition. Eligibility getters run inside the shared scope. |

A missing/noncallable optional handler still consumes its dispatch unit. Script
instructions, budgeted native calls and nested event types use the same counter.
A new outer host operation resets its counter; nested operations cannot reset it.
Invalid argument/version checks retain their order. These units are neither time
nor bytes; arbitrary host callbacks and other native subsystems acquire no
implicit time limits. AsBroadcaster retains its [own accounting](AS_BROADCASTER.md#registration-work-limits-v0617).

## Errors, publication and ownership

Exhaustion or observed worker stop returns [`FVM_ERROR`](../include/flashvm/flashvm.h#L43). Add/remove precharge all
movement before mutation: failure preserves registration identity/order/GC roots.
Native remove also preserves `removed` on errors. Successful return/boolean
contracts remain unchanged. Allocation failure remains [`FVM_OUT_OF_MEMORY`](../include/flashvm/flashvm.h#L44).

Snapshot/publication preparation errors deliver nothing and preserve valid input
and focus. Existing focus validation can still clear an already removed clip.
After publication, failure preserves new keyboard/pointer/focus state, drag
changes and completed callback effects, then skips remaining recipients. It does
not roll back getters or earlier handlers. Explicit native setters/reset keep
their silent contracts. No retry/replay is automatic.

Recipient snapshots remain strong GC roots through unwinding. Mutations affect
later events. Collection inside a rooted handler follows its normal contract.
Pointer ownership, ABI, registration bounds, signatures and cross-thread rules
are unchanged. Native registration and dispatch functions are declared in
[keyboard.h](../include/flashvm/keyboard.h) and [mouse.h](../include/flashvm/mouse.h).

Zero disables the numeric limit, but cooperative worker cancellation remains
active. A stop is checked on the first charged unit and periodically every 64
charged units. More callbacks may complete before it is observed; this cannot
interrupt an arbitrary blocking host callback. Worker lifecycle ownership:
[worker.h](../include/flashvm/worker.h).

## Verification and limits

- `input_listener_budget`: 135 SWF6–8/namespace/call-route/failure combinations,
  failed detached inspection, retry after GC and nested shared budgets.
- `input_dispatch_budget`: preparation rollback, committed delivery errors,
  nested native work and GC for keyboard, pointer, wheel and focus.
- `jit_input_event_budget`: actual SWF6–8 script handlers in three execution
  modes; actual native entries are checked when JIT is available.
- `input_worker_cancellation`: ten real OS workers, five event routes with a
  stop before preparation and in the first handler, with numeric limits disabled.

Existing listener/allocation/focus/wheel/queue/JIT tests remain relevant. These
are Linux x86-64 regressions, not full Flash/reference, device, remote/endurance
or Windows/macOS/Android acceptance. All nine roadmap stages remain open.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_Config` | struct | [include/flashvm/flashvm.h:51](../include/flashvm/flashvm.h#L51) |
| `FVM_ERROR` | enum constant | [include/flashvm/flashvm.h:43](../include/flashvm/flashvm.h#L43) |
| `FVM_OUT_OF_MEMORY` | enum constant | [include/flashvm/flashvm.h:44](../include/flashvm/flashvm.h#L44) |
<!-- source-links:end -->
