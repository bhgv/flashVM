# Explicitly polled AS1/AS2 timers — v0.621

[Русский](TIMERS.ru.md). [fvm_standard_install](../include/flashvm/standard.h#L19) publishes `setInterval`,
`clearInterval` (host/SWF6–8), `setTimeout`, `clearTimeout` (host/SWF8).
Availability is checked on each call, including installation before SWF loading.
These functions use the existing [context clock](GET_TIMER.md). The public
[fvm_timers_poll](../include/flashvm/timers.h#L19) supplies explicit owning-thread delivery; the common desktop
player polls once after each timeline tick, before rendering.

## Registration and cancellation

Supported forms:

```actionscript
var interval = setInterval(callbackFunction, 100, arg1, arg2);
var methodInterval = setInterval(object, "methodName", 100, arg1);
var timeout = setTimeout(callbackFunction, 100, arg1);
clearInterval(interval);
clearTimeout(timeout);
```

The target must be a same-context invocable Function, or for the interval method
form a same-context noninvocable Object with a nonempty String method name.
Named methods are resolved at delivery, so replacing the property changes the
next callback; its receiver is the supplied object. Counted method names follow
the existing member-call service, including active SWF case rules. Callable
objects select the Function form. The object/method timeout form and coercible
non-String method names are [FVM_UNSUPPORTED](../include/flashvm/flashvm.h#L46). Missing required arguments and
invalid/foreign targets are [FVM_INVALID_ARGUMENT](../include/flashvm/flashvm.h#L45).

Delay uses AVM1 numeric coercion, which may call user code and collect. Supported
finite delays are [FVM_TIMERS_MIN_DELAY](../src/standard/timer_constants.h#L6) through [FVM_TIMERS_MAX_DELAY](../src/standard/timer_constants.h#L7); fractions
round down after range validation. Zero, negative, NaN, infinity, submillisecond
and larger delays are explicitly [FVM_UNSUPPORTED](../include/flashvm/flashvm.h#L46). This is a bounded profile,
not an assertion of original-player clamping. At most [FVM_TIMERS_MAX_ARGUMENTS](../src/standard/timer_constants.h#L5)
extra values and [FVM_TIMERS_MAX_PENDING](../src/standard/timer_constants.h#L4) live entries are supported. Capacity or
exhausted IDs return [FVM_ERROR](../include/flashvm/flashvm.h#L43); too many extra arguments return [FVM_UNSUPPORTED](../include/flashvm/flashvm.h#L46).

Arguments are shallow snapshots; referenced objects remain live shared objects.
Borrowed strings, including counted NUL, are copied into context storage. The
managed Function/capture, method owner/name, argument objects, primitive
metatables and traced strings remain strong roots until cancellation/consumption.
Native callback userdata is borrowed host memory and must outlive every pending
or active call. Cancel explicitly before releasing it, or destroy the context.

Successful registration returns an exact positive Number ID up to
[FVM_TIMERS_MAX_ID](../src/standard/timer_constants.h#L8). IDs never wrap or reuse within a context. IDs are meaningful
only in that context; the four globals use one shared queue/ID namespace.
Cancellation converts the first argument to Number. Absent, fractional,
nonfinite, out-of-range and unknown IDs have no effect; result is undefined.
Either clear function cancels either kind under its own SWF version gate.
Registration and cancellation failures leave the queue/ID counter unchanged
apart from explicit side effects of user coercion. Registration/cancellation errors yield
undefined outputs through ordinary global/Function invocation. Clock sampling may update
the existing clock state before a later allocation/work failure.

## fvm_timers_poll contract

Pass a live owning-thread context and non-null `unsigned int *delivered`.
Reentry, destruction, collection and release phases return [FVM_INVALID_ARGUMENT](../include/flashvm/flashvm.h#L45)
without changing the count. A valid call resets the count to zero; an empty queue
returns immediately without reading the clock. Otherwise it samples the clock
once and snapshots due IDs in registration order. Clock errors return [FVM_ERROR](../include/flashvm/flashvm.h#L43).
A timer is eligible when its unwrapped deadline is at or before that sample.

Each captured ID is attempted at most once. Cancelled entries are skipped; new
registrations wait for another poll, even if a callback advances the clock.
Before invocation a timeout is deleted; an interval is rearmed at sample+delay.
Late intervals coalesce into one delivery, with no catch-up burst. A deadline
that cannot advance at floating-point precision is [FVM_UNSUPPORTED](../include/flashvm/flashvm.h#L46). The existing
unsigned GetTime wrap does not wrap scheduler deadlines.

The count includes a committed delivery whose invocation later fails, including
OOM while preparing its Function activation. Any callback status/exception stops
the batch; consumed timeouts, rearmed intervals, completed callbacks and explicit
user side effects remain committed. Before commitment, snapshot/allocation/work
failures leave the corresponding pending entry unchanged. An interval remains
pending after its callback fails unless user code cancelled it. Clear a pending
exception with [fvm_exception_clear](../include/flashvm/exception.h#L65) before continuing after [FVM_EXCEPTION](../include/flashvm/flashvm.h#L48).

[GC:] An active value-vector root is installed before deletion/rearming, so a
callback may cancel itself, register/cancel other timers and collect safely.
Recursive [fvm_timers_poll](../include/flashvm/timers.h#L19) is rejected; [fvm_context_destroy](../include/flashvm/flashvm.h#L70) during dispatch is
ignored, as for existing input dispatch. There is no sleeping, OS timer/thread,
preemption of arbitrary native callbacks or implicit [fvm_advance](../include/flashvm/flashvm.h#L74) implementation.
The host determines polling frequency. Desktop source integration is not foreign
OS, visible playback or real-device acceptance.

## Work, graphs and evidence

[FVM_ExecutionWorkScope](../src/core/execution_work_scope.hpp#L9) shares the caller's instruction budget and worker stop
polling with registration, numeric coercion, queue scanning and callback work.
Registration charges each extra argument, each retained string byte and publication;
cancellation charges lookup and value/map removal; polling charges queue scan,
delivery and active-value copying/removal. Preparation stops before queue mutation
on exhausted work. Native callback wall time is not preempted. Numeric limit zero
keeps cooperative worker cancellation enabled.

Extracted scheduling globals serialize with stable callback IDs 164–167 after
getTimer 163. Restored globals retain rooted collecting coercion. Pending queues,
IDs, deadlines, native userdata and live native bodies are not exported with a
Function graph; an imported registration uses the destination context's queue.
Context destruction drops pending roots before finalization without invoking them.

Regression sources: [basic/versions](../tests/timers.cpp),
[GC/reentry/errors](../tests/timers_gc_reentry.cpp),
[budget/capacity](../tests/timers_budget.cpp), [OOM](../tests/timers_allocation.cpp),
[graph/coercion](../tests/timers_graph.cpp), [AVM1/JIT](../tests/jit_timers.cpp),
[version/conflict guards](../tests/timers_versions_conflicts.cpp),
[real workers](../tests/timers_worker_cancellation.cpp).
[C89 tutorial](../examples/embedding/timers/README.md).

The [Adobe AS2 reference](https://open-flash.github.io/mirrors/as2-language-reference/global_functions.html)
establishes version availability, Function/object-method interval forms, callback
arguments and one-shot timeout purpose. Exact original-player scheduling,
frame-rate rounding, invalid-delay recovery, receiver edge cases, movie teardown
and cross-platform/endurance acceptance remain open. This work closes no entire
roadmap stage.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_advance` | function | [include/flashvm/flashvm.h:74](../include/flashvm/flashvm.h#L74) |
| `fvm_context_destroy` | function | [include/flashvm/flashvm.h:70](../include/flashvm/flashvm.h#L70) |
| `FVM_ERROR` | enum constant | [include/flashvm/flashvm.h:43](../include/flashvm/flashvm.h#L43) |
| `FVM_EXCEPTION` | enum constant | [include/flashvm/flashvm.h:48](../include/flashvm/flashvm.h#L48) |
| `fvm_exception_clear` | function | [include/flashvm/exception.h:65](../include/flashvm/exception.h#L65) |
| `FVM_ExecutionWorkScope` | class | [src/core/execution_work_scope.hpp:9](../src/core/execution_work_scope.hpp#L9) |
| `FVM_INVALID_ARGUMENT` | enum constant | [include/flashvm/flashvm.h:45](../include/flashvm/flashvm.h#L45) |
| `fvm_standard_install` | function | [include/flashvm/standard.h:19](../include/flashvm/standard.h#L19) |
| `FVM_TIMERS_MAX_ARGUMENTS` | macro / define | [src/standard/timer_constants.h:5](../src/standard/timer_constants.h#L5) |
| `FVM_TIMERS_MAX_DELAY` | macro / define | [src/standard/timer_constants.h:7](../src/standard/timer_constants.h#L7) |
| `FVM_TIMERS_MAX_ID` | macro / define | [src/standard/timer_constants.h:8](../src/standard/timer_constants.h#L8) |
| `FVM_TIMERS_MAX_PENDING` | macro / define | [src/standard/timer_constants.h:4](../src/standard/timer_constants.h#L4) |
| `FVM_TIMERS_MIN_DELAY` | macro / define | [src/standard/timer_constants.h:6](../src/standard/timer_constants.h#L6) |
| `fvm_timers_poll` | function | [include/flashvm/timers.h:19](../include/flashvm/timers.h#L19) |
| `FVM_UNSUPPORTED` | enum constant | [include/flashvm/flashvm.h:46](../include/flashvm/flashvm.h#L46) |
<!-- source-links:end -->
