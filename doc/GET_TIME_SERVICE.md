# GetTime service / Монотонный таймер — v0.291

Action GetTime and C89 `fvm_exec_get_time(context, output)` now return integer
milliseconds relative to context creation, modulo 2^32. The service takes no
bytecode or interpreter stack and allocates no VM memory. Loading another SWF
does not reset the context origin. Different contexts retain separate origins.
The result is an unsigned-width Number, including values above signed Int32.

The clock adapter is separate from Date's calendar clock. A validated finite,
nonnegative monotonic sample establishes the origin. Elapsed samples cannot
move backwards; only the returned 32-bit representation wraps. Failed or invalid
reads preserve output and prior elapsed state. If the initial clock read fails,
context creation still succeeds; the first successful service read establishes
an origin and returns zero. These timer fields are context-local and are not
serialized as object state. Destination contexts retain their own clock origins.

Ports use CLOCK_MONOTONIC on Linux/Android, QueryPerformanceCounter/Frequency on
Windows and mach_absolute_time/timebase conversion on macOS. Clock unit constants
are named in ports/time_constants.h. Only the Linux adapter has been built and
run here; other platform sources await actual target builds. Suspend/resume
behavior and historical player timing remain compatibility gates. No emulator
busy-loop time acceleration or debugger time-freezing policy is introduced.

Primary semantic comparison: pinned Ruffle
[action_get_time](https://github.com/ruffle-rs/ruffle/blob/2372a4e148a67d68efd796f4477b3cddd4f880cd/core/src/avm1/activation.rs).
Adapter references:
[POSIX monotonic clocks](https://pubs.opengroup.org/onlinepubs/000095399/functions/clock_getres.html),
[Windows performance counter](https://learn.microsoft.com/en-us/windows/win32/api/profileapi/nf-profileapi-queryperformancecounter),
[Windows frequency](https://learn.microsoft.com/en-us/windows/win32/api/profileapi/nf-profileapi-queryperformancefrequency),
[Mach clock/timebase](https://developer.apple.com/library/archive/qa/qa1643/_index.html).

The deterministic C89 fixture replaces only the link-time monotonic adapter and
checks SWF4–8 Action dispatch, context origins, load preservation, fractional
truncation, 32-bit wrap, backward samples, invalid/failing reads and recovery.
A separate fixture exercises the actual Linux adapter. Allocation injection
confirms reads succeed with all VM allocations disabled, without changing GC
root chains or object counts.

Добавлен Action GetTime и общий C-интерфейс. Начало отсчёта привязано к контексту,
загрузка SWF его не сбрасывает. Календарные часы Date не изменены. При ошибке часов
выход и накопленное время сохраняются; восстановление после недоступности часов
проверено. Дробная часть отбрасывается, результат оборачивается по модулю 2^32.
[GC:] Обработчик не выделяет память и не требует временных корней.
Адаптер Linux проверен реально; Windows/macOS/Android пока представлены исходниками,
поэтому завершение платформенной стадии и всех стадий 1–9 не заявляется.
