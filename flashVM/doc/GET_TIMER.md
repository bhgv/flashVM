# getTimer — bounded context clock (v0.620)

[Русский](GET_TIMER.ru.md). Standard installation publishes `getTimer()` in host
contexts and loaded SWF4–8. It delegates to [fvm_exec_get_time](../include/flashvm/execution.h#L168), the same service
used by AVM1 GetTime and generated code. Extra arguments are ignored without
ToString/valueOf. Extracted Functions and ordinary global calls share the clock.

## Clock and limits

The current clock origin is context creation, not loading, advancing or starting
a movie. A failed initial read is recovered by the first successful read, which
establishes zero. Valid finite nonnegative platform samples produce elapsed
integer milliseconds, rounded down and reduced modulo 2^32. Clock rollback
retains the previous unwrapped elapsed value; it cannot reduce time except at
modulo wrap. The unsigned result includes values above 2^31. This preserves the
existing execution-service contract; exact original-player movie-start and
signed-wrap behavior remains unaccepted. [fvm_advance](../include/flashvm/flashvm.h#L74) does not advance this clock.

No sleep, scheduler, interval, timeout, network or device is created. Contexts
have separate origins; calls belong on the owning VM thread. This change does
not implement `setInterval`, `clearInterval` or `setTimeout`.

## Calls, errors and ownership

Call [fvm_standard_install](../include/flashvm/standard.h#L19) before [fvm_call](../include/flashvm/value.h#L138) with name `getTimer`, zero arguments
and a non-null output. Installation preserves conflicting own names using active
SWF case rules and returns [FVM_INVALID_ARGUMENT](../include/flashvm/flashvm.h#L45); repeated installation validates
registered identities. Loaded versions below SWF4 are unsupported by the callback.
Direct [fvm_call](../include/flashvm/value.h#L138) and extracted [fvm_function_invoke](../include/flashvm/function.h#L65) return a plain Number on success,
[FVM_ERROR](../include/flashvm/flashvm.h#L43) for unavailable/invalid platform samples, and undefined output on failure.
The private callback rejects destruction, collection and release phases before
reading the clock. It performs no allocation or user coercion; ordinary call and
Function machinery may allocate. Existing native-call barriers remain in force.
The public [fvm_exec_get_time](../include/flashvm/execution.h#L168) instead preserves its supplied output on errors.

[GC:] Extracted Functions obey ordinary managed-object lifetime rules: retain them
in a rooted binding or pin with [fvm_gc_pin](../include/flashvm/gc.h#L48) across collection, then release with
[fvm_gc_unpin](../include/flashvm/gc.h#L54). Numeric results own no managed memory. Serialization assigns the
new callback ID 163 without changing IDs 150–162; after sender teardown an
imported Function reads the destination context's clock, never the sender's
origin/sample. No origin or platform handle is embedded in the Function graph.

Implementation: [get_timer_call](../src/standard/numeric.cpp#L700); stable identity: [FVM_TIME_GET_TIMER_CALLBACK](../src/standard/time_constants.h#L5).

## Evidence

[tests/global_get_timer.cpp](../tests/global_get_timer.cpp) injects the platform clock at link time and checks
host/SWF4–8 global and Function routes, fractions, rollback, 2^31/2^32 boundaries,
failed/invalid reads, recovery, ignored coercible objects, phase guards,
case-sensitive installation and graph transfer after sender destruction.
[tests/jit_get_timer.cpp](../tests/jit_get_timer.cpp) compares GetTime, global calls and aliases in interpreter,
manual native JIT and automatic JIT modes for SWF4–8, checking native entry counts.
[tests/global_get_timer_allocation.cpp](../tests/global_get_timer_allocation.cpp) separately exercises allocation failure.
[C89 tutorial](../examples/embedding/get_timer/README.md).

Reference: [Adobe AS2 global-functions reference](https://open-flash.github.io/mirrors/as2-language-reference/global_functions.html),
getTimer entry: Player 4+, integer milliseconds since the SWF started playing.
It establishes intended availability and purpose, not acceptance of this
implementation's lifetime/wrap policy or any unexecuted platform.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_advance` | function | [include/flashvm/flashvm.h:74](../include/flashvm/flashvm.h#L74) |
| `fvm_call` | function | [include/flashvm/value.h:138](../include/flashvm/value.h#L138) |
| `FVM_ERROR` | enum constant | [include/flashvm/flashvm.h:43](../include/flashvm/flashvm.h#L43) |
| `fvm_exec_get_time` | function | [include/flashvm/execution.h:168](../include/flashvm/execution.h#L168) |
| `fvm_function_invoke` | function | [include/flashvm/function.h:65](../include/flashvm/function.h#L65) |
| `fvm_gc_pin` | function | [include/flashvm/gc.h:48](../include/flashvm/gc.h#L48) |
| `fvm_gc_unpin` | function | [include/flashvm/gc.h:54](../include/flashvm/gc.h#L54) |
| `FVM_INVALID_ARGUMENT` | enum constant | [include/flashvm/flashvm.h:45](../include/flashvm/flashvm.h#L45) |
| `fvm_standard_install` | function | [include/flashvm/standard.h:19](../include/flashvm/standard.h#L19) |
| `FVM_TIME_GET_TIMER_CALLBACK` | macro / define | [src/standard/time_constants.h:5](../src/standard/time_constants.h#L5) |
<!-- source-links:end -->
