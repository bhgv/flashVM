# Polled callbacks and cancellation

Executable C89 example using the real context clock, a native Function and the
AS1/AS2 scheduling globals. No SWF, network, display or worker is required.
[Русский](README.ru.md). [Complete contract](../../../doc/TIMERS.md).

Build/run from the FlashVM component:

```sh
cmake -S . -B build -DFVM_BUILD_EXAMPLES=ON
cmake --build build --target flashvm_examples_embedding_timers_main
./build/flashvm_examples_embedding_timers_main
./build/flashvm_examples_embedding_timers_main --wait
```

Default mode polls once then cancels, without waiting. `--wait` prompts for Enter;
wait at least one second to make the interval due. Expected output:
`cancelled interval; callbacks: N; attempted: N`, normally N=0 by default and N=1
in interactive mode. Slow setup/quick input can change eligibility; a poll still
attempts at most one delivery of this interval. CTest uses the default mode.

Before polling it prints `next poll in at most N ms (pending=1)`, with N between
zero and the 1000 ms ceiling. A query does not invoke the callback; after
cancellation the example verifies pending=0 and the unchanged host ceiling.
[Wakeup contract](../../../doc/TIMER_WAKEUP.md).

The interval callback calls updateAfterEvent; after polling, the host observes and
consumes its redraw flag, printing `callback redraw: N` (normally 0/1 as above).
The example also verifies that two explicit host requests coalesce into one.
It requires no renderer and never claims a display was updated.
[Redraw contract](../../../doc/UPDATE_AFTER_EVENT.md).

## API, lifetime and blocking

| Function | Demonstrated contract |
| --- | --- |
| [fvm_config_init](../../../include/flashvm/flashvm.h#L70) | Initialize caller-owned configuration ABI defaults. |
| [fvm_runtime_create](../../../include/flashvm/flashvm.h#L74) | Copy configuration and acquire the runtime; outlives its context. |
| [fvm_context_create](../../../include/flashvm/flashvm.h#L82) | Acquire the owning-thread heap and independent monotonic origin. |
| [fvm_standard_install](../../../include/flashvm/standard.h#L22) | Publish scheduling globals transactionally, preserving conflicting own bindings. |
| [fvm_function_create_native](../../../include/flashvm/function.h#L57) | Create a managed Function with borrowed callback code/userdata; pending registration retains it. |
| [fvm_value_undefined](../../../include/flashvm/value.h#L46) | Initialize Function value fields or produce the ignored callback result; allocates no buffer. |
| [fvm_value_number](../../../include/flashvm/value.h#L50) | Prepare the delay Number; no managed storage is acquired. |
| [fvm_call](../../../include/flashvm/value.h#L204) | Synchronously register/cancel via setInterval/clearInterval; may allocate/coerce, but does not wait for due time. |
| [fvm_timers_next_delay](../../../include/flashvm/timers.h#L34) | Read an allocation-free capped wakeup hint without callbacks; preserve both outputs on errors and return the host ceiling for an empty queue. |
| [fvm_timers_poll](../../../include/flashvm/timers.h#L22) | Attempt the due snapshot synchronously; may call user code/allocate and returns attempted count, including a failing delivery. |
| [fvm_stage_request_update](../../../include/flashvm/stage.h#L52) | Coalesce an explicit host redraw independent of script-event eligibility; allocate/call/render nothing. |
| [fvm_stage_get_update](../../../include/flashvm/stage.h#L57) | Read the pending plain flag without clearing it; owner-thread and lifecycle restrictions apply. |
| [fvm_stage_take_update](../../../include/flashvm/stage.h#L63) | Read and clear before host drawing, preserving later requests; request again if a failed presentation is retried. |
| [fvm_context_destroy](../../../include/flashvm/flashvm.h#L86) | Drop pending roots and free the heap before host callback counters leave scope. |
| [fvm_runtime_destroy](../../../include/flashvm/flashvm.h#L78) | Release the outer owner after all contexts have been destroyed. |

The local counters outlive pending and active callbacks. No explicit pin is needed:
there is no GC between Function construction and registration; the queue then
retains it. Numeric IDs own no buffer. Errors share one cleanup path, which also
releases a still-pending registration. All VM operations stay on the owner thread.
Only the optional host `getchar()` waits; the VM neither sleeps nor starts a timer
thread. Ordinary native callbacks retain their GC barrier. This tutorial does not
certify original-player scheduling, device playback or other OS runs.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_call` | function | [include/flashvm/value.h:204](../../../include/flashvm/value.h#L204) |
| `fvm_config_init` | function | [include/flashvm/flashvm.h:70](../../../include/flashvm/flashvm.h#L70) |
| `fvm_context_create` | function | [include/flashvm/flashvm.h:82](../../../include/flashvm/flashvm.h#L82) |
| `fvm_context_destroy` | function | [include/flashvm/flashvm.h:86](../../../include/flashvm/flashvm.h#L86) |
| `fvm_function_create_native` | function | [include/flashvm/function.h:57](../../../include/flashvm/function.h#L57) |
| `fvm_runtime_create` | function | [include/flashvm/flashvm.h:74](../../../include/flashvm/flashvm.h#L74) |
| `fvm_runtime_destroy` | function | [include/flashvm/flashvm.h:78](../../../include/flashvm/flashvm.h#L78) |
| `fvm_stage_get_update` | function | [include/flashvm/stage.h:57](../../../include/flashvm/stage.h#L57) |
| `fvm_stage_request_update` | function | [include/flashvm/stage.h:52](../../../include/flashvm/stage.h#L52) |
| `fvm_stage_take_update` | function | [include/flashvm/stage.h:63](../../../include/flashvm/stage.h#L63) |
| `fvm_standard_install` | function | [include/flashvm/standard.h:22](../../../include/flashvm/standard.h#L22) |
| `fvm_timers_next_delay` | function | [include/flashvm/timers.h:34](../../../include/flashvm/timers.h#L34) |
| `fvm_timers_poll` | function | [include/flashvm/timers.h:22](../../../include/flashvm/timers.h#L22) |
| `fvm_value_number` | function | [include/flashvm/value.h:50](../../../include/flashvm/value.h#L50) |
| `fvm_value_undefined` | function | [include/flashvm/value.h:46](../../../include/flashvm/value.h#L46) |
<!-- source-links:end -->
