# Frame availability / Проверка доступности кадра — v0.293

WaitForFrame and WaitForFrame2 now call C89 `fvm_exec_frame_loaded` with semantic
DIRECT or VALUE policies. The service accepts values and returns Boolean without
bytecode or an interpreter stack. The interpreter decodes the direct frame/skip
fields and skips complete Action records when the result is false. Variable-size
payloads and nested With bodies each count as one outer Action. Skipping through
or beyond the terminal record ends the block successfully. Skipped records do
not execute, including otherwise unsupported Actions.

DIRECT accepts an already decoded, unsigned integral frame request. Values above
16000 report unavailable. VALUE takes finite integral Numbers directly; otherwise
it stringifies once and accepts only a complete integral decimal float spelling.
Labels, fractional strings, whitespace, radix prefixes, special values and
embedded NUL suffixes become zero. The numeric result wraps to signed Int32;
the historical wrapping subtract/saturating add maps Int32 minimum to maximum.
VALUE permits 16001 and compares native frames-loaded plus one against the
unsigned-16-bit request clamped to header frames. Bounds are checked even when
no target exists; absent/non-MovieClip targets otherwise report ready.

[GC:] Original and converted values stay rooted across toString, collection and
exceptions. Availability reads the selected target after coercion and bypasses
replaceable _framesloaded/_totalframes getters. A callback activation restores
its own target scope on return. Output may alias input and stays unchanged on
failure. Allocation failure and root restoration are covered.

The current scene loader imports complete in-memory timelines; its intrinsic
frames-loaded equals total frames. This change does not add partial downloads,
streaming frame availability or loader events. Those remain scene/API roadmap
work. Historical player edge cases and full Action equivalence remain open.

Primary comparison: pinned Ruffle
[WaitForFrame/WaitForFrame2](https://github.com/ruffle-rs/ruffle/blob/2372a4e148a67d68efd796f4477b3cddd4f880cd/core/src/avm1/activation.rs).
C89 and Action fixtures cover SWF4–8, direct/value limits, strict decimal parsing,
Int32 wrapping, counted NUL, missing operands, complete-record skips, unknown
payloads, nested bodies, terminal overshoot, malformed payloads, GC, exceptions
and native getter bypass. The dispatch catalog now has 94 present references
and six absent entries; that count is not a compatibility certification.

Оба Action-кода подключены к общему C-интерфейсу. Пропуск считается в записях
Action, а не в байтах; выход за конец блока завершает его нормально. Разные
границы и правила преобразования двух вариантов сохранены. Проверки охватывают
GC внутри toString, исключения, отказы памяти и восстановление области цели.
Текущие SWF загружаются целиком: потоковая загрузка и её события не реализованы
этим изменением. Все стадии 1–9 остаются открытыми.
