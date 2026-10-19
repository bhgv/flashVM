# Host timer wakeup hints — v0.622

[Русский](TIMER_WAKEUP.ru.md). [fvm_timers_next_delay](../include/flashvm/timers.h#L29) lets an event loop cap its
ordinary wait at the next scheduled callback. It reads the same unwrapped
[context clock](GET_TIMER.md) and queue as [fvm_timers_poll](../include/flashvm/timers.h#L19), without invoking or
consuming a timer. Registration/delivery limits remain in [TIMERS.md](TIMERS.md).

## API and ownership

Call [fvm_timers_next_delay](../include/flashvm/timers.h#L29) on the context owner thread, passing a host ceiling
`max_wait_ms`, a non-null `unsigned long *wait_ms` and a non-null `int *pending`.
Choose a ceiling representable by the host's wait API. No memory is allocated,
no user callback/coercion executes, and no managed result needs pinning.

For an empty queue it returns [FVM_OK](../include/flashvm/flashvm.h#L42), wait=max_wait_ms and pending=0, without
reading the platform clock. For a nonempty queue pending=1 and wait is the
smaller of the ceiling and the earliest remaining delay rounded upward to an
integer millisecond. Due timers return zero. A positive fractional remainder
returns at least one unless the supplied ceiling is zero. Zero and ULONG_MAX
ceilings are supported; an empty queue returns even ULONG_MAX exactly.

[FVM_INVALID_ARGUMENT](../include/flashvm/flashvm.h#L45) rejects null context/outputs and destruction, collection or
release phases. Clock failure/invalid samples or exhausted execution work return
[FVM_ERROR](../include/flashvm/flashvm.h#L43). Every error preserves both outputs and all pending records/deadlines.
Clock sampling may advance the existing retained elapsed sample before a later
work error. Rollback and modulo-2^32 GetTime output do not wrap deadlines.

The query may run inside a timer callback; it shares the caller's
[FVM_ExecutionWorkScope](../src/core/execution_work_scope.hpp#L9) rather than resetting the budget. One work unit is charged
per pending entry scanned. Numeric limit zero keeps cooperative worker stop
polling active. The query never short-circuits a due entry to bypass charging
later queue entries. It creates no temporary GC root, string, Function or OS timer.

## Event-loop use

Poll ready work; query with the host's next-event/frame/audio wait ceiling;
then wait for an OS event or that capped duration. Process events and repeat.
A zero hint means retry delivery without blocking, not recursively poll from a
callback. Query again after registration/cancellation or other queue changes.
The hint is a snapshot, not an OS wakeup registration or a realtime deadline
guarantee. [fvm_timers_poll](../include/flashvm/timers.h#L19) still rejects recursive delivery.

The common desktop player now pumps input and polls timers on active event-loop
iterations, including between timeline frames. SDL waiting is capped by the
nearest timer as well as the existing frame/audio limits. Callback activations
can be collected at a safe host boundary between slow frames. Timeline advance,
rendering and frame counters keep their authored cadence; callback state changes
appear at the next render. This does not implement updateAfterEvent.

Existing background suspension remains: an inactive player waits without timer
delivery; on resumption late intervals coalesce under the existing policy.
The bounded final-frame display wait also does not continue the active timeline
or scheduled delivery. Exact original-player timing/lifecycle and real-device
acceptance remain open.

## Evidence

[Clock/budget/lifecycle test](../tests/timers_next_delay.cpp) covers upward rounding,
ceilings, empty queues with failed clocks, errors without output changes,
rollback, GetTime wrap, large-double deadline rounding, callbacks querying during
delivery and shared budgets.
[Allocation test](../tests/timers_next_delay_allocation.cpp) disables all new/new[]
allocations across repeated queries and checks unchanged roots, objects, strings,
deadlines and callback counts. [Real workers](../tests/timers_worker_cancellation.cpp)
exercise query cancellation before polling and from the first callback.

[Player regression](../frontend/tests/timers.py) runs the actual SDL dummy-video
player with SWF6–8 at 2 fps. A 10 ms interval must deliver at least three callbacks
before the second frame, in interpreter and native JIT modes. A damaged count
fixture must fail. A temporary player linked with the v0.621 frame-only loop
also failed the positive movie, establishing that this test detects the prior
behavior. These are protocol/timing-window checks, not latency benchmarks or
visible-window/audio/foreign-OS acceptance.

[C89 tutorial](../examples/embedding/timers/README.md) demonstrates querying before
poll and verifying an empty queue after cancellation. The VM stays independent
of SDL; any host can use the C89 query with its own event source.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_ERROR` | enum constant | [include/flashvm/flashvm.h:43](../include/flashvm/flashvm.h#L43) |
| `FVM_ExecutionWorkScope` | class | [src/core/execution_work_scope.hpp:9](../src/core/execution_work_scope.hpp#L9) |
| `FVM_INVALID_ARGUMENT` | enum constant | [include/flashvm/flashvm.h:45](../include/flashvm/flashvm.h#L45) |
| `FVM_OK` | enum constant | [include/flashvm/flashvm.h:42](../include/flashvm/flashvm.h#L42) |
| `fvm_timers_next_delay` | function | [include/flashvm/timers.h:29](../include/flashvm/timers.h#L29) |
| `fvm_timers_poll` | function | [include/flashvm/timers.h:19](../include/flashvm/timers.h#L19) |
<!-- source-links:end -->
