# Frame-free callbacks / Внекадровые callbacks

`Async.post(callback, ...args)` queues work for a later owner-loop poll;
`Async.cancel(id)` cancels it. Hosts opt in with
[fvm_async_install](../include/flashvm/async.h#L14). C89 callers can use
[`fvm_async_post`](../include/flashvm/async.h#L23) and [`fvm_async_cancel`](../include/flashvm/async.h#L30) directly.

Delivery uses [fvm_timers_poll](../include/flashvm/timers.h#L22), independently of
SWF frame advancement. A posted callback is eligible on the next poll even if
the sampled clock has not advanced. Posts inside callbacks wait for another
poll; nested polls reject. The host must pump while the movie is paused too.
[`fvm_timers_next_delay`](../include/flashvm/timers.h#L34) returns zero for eligible work. There is no worker-thread
execution of ActionScript or concurrent mutation of its heap.

Pending callbacks and copied arguments are GC roots. Cancellation and context
teardown release them. IDs share the existing timer namespace; queue, argument,
execution-work and clock limits remain those of the timer scheduler. Errors stop
delivery and propagate to the host. Installation preserves existing own Async
globals by rejecting conflicts. Native method serialization is unsupported.
Legacy setTimeout/setInterval version and delay rules remain unchanged.

IDE integration posts one paint notification for the latest acknowledged Dis
state. Native raster workers still receive detached immutable commands, and
pointer events retain the displayed-geometry barrier. Live SDL text input merges
already queued same-window text and ordinary letter transitions into a bounded
UTF-8 edit. Ctrl/Alt/GUI, modifiers, focus, pointers, IME and capacity boundaries
remain ordered. Native letter-down state is retained; no-op letter key messages
no longer force a full Dis view. A queued typing burst becomes one Undo edit.
Replay/capture keeps deterministic presentation ordering.

Tests: `tests/async.cpp` freezes the clock and advances no movie frames; it checks
GC roots, copied arguments, cancellation, script method dispatch and next-poll
reposting. `disVM/runtime/test_text_events.c` checks actual SDL FIFO batching:
51 characters become one message, with shortcut/window/capacity barriers intact.
These are executable regressions, not a physical keyboard latency benchmark.

Русское описание: расширение называется **Async**. post откладывает callback до
следующего прохода владельца VM, cancel отменяет его. Кадр SWF для исполнения
не требуется; автоматического фонового потока нет. Владелец вызывает poll и при
остановленной timeline. Аргументы копируются и удерживаются GC; новые callbacks
из callback ждут следующего poll. IDE объединяет накопившийся текст, сохраняет
порядок команд и готовит только последнее подтверждённое состояние. Несколько
символов пакета образуют одну операцию Undo. Физическая задержка на устройстве
пока не измерялась.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_async_cancel` | function | [include/flashvm/async.h:30](../include/flashvm/async.h#L30) |
| `fvm_async_install` | function | [include/flashvm/async.h:14](../include/flashvm/async.h#L14) |
| `fvm_async_post` | function | [include/flashvm/async.h:23](../include/flashvm/async.h#L23) |
| `fvm_timers_next_delay` | function | [include/flashvm/timers.h:34](../include/flashvm/timers.h#L34) |
| `fvm_timers_poll` | function | [include/flashvm/timers.h:22](../include/flashvm/timers.h#L22) |
<!-- source-links:end -->
