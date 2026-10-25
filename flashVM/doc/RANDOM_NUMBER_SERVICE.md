# RandomNumber service / Ограниченное случайное число — v0.290

C89 `fvm_exec_random_number(context, maximum, output)` implements Action
RandomNumber through a value-based interface. The interpreter only supplies its
operand and publishes the result. Loaded empty stacks act as undefined input;
unloaded empty stacks retain the existing strict execution policy.

The bound is numerically converted once under the SWF version policy, truncated
toward zero and saturated to the nonnegative signed-Int32 range. NaN, negative
numbers and positive fractions below one return zero without drawing. Positive
infinity and larger finite bounds saturate at Int32 maximum; they do not wrap
modulo 2^32. Positive bounds draw one integer and use remainder by the bound.
Output may alias input. Failed preparation preserves output and the generator
state, except for user callback effects that have already occurred.

[GC:] Common number conversion roots the original operand through valueOf,
collection and exceptions. No random draw occurs until conversion succeeds.
A callback may itself call Math.random; those draws remain visible after a
subsequent exception. Allocation injection covers alias/root/state preservation.

Math.random and RandomNumber share the existing context-local 32-bit LCG and
initial seed through core/random.hpp and documented random_constants.h. Existing
Math.random sequences are unchanged. The existing Math graph codec captures this
same state: a Math snapshot round trip resumes RandomNumber at the same point.
No graph schema or resource identity is added. The generator is FlashVM's
existing deterministic implementation; exact historical player sequences and
locale-dependent RNG behavior are not claimed.

Primary semantic comparison: pinned Ruffle
[action_random_number](https://github.com/ruffle-rs/ruffle/blob/2372a4e148a67d68efd796f4477b3cddd4f880cd/core/src/avm1/activation.rs).
Fixtures cover SWF4–8, actual Action dispatch, numeric/string/special bounds,
Int32 saturation, Math interleaving, state transport, otherwise-unrooted receivers,
collecting/throwing callbacks and allocation failures.

Добавлен отсутствовавший Action RandomNumber и общий C-обработчик. Верхняя
граница преобразуется один раз и ограничивается диапазоном Int32 без оборачивания.
Для нулевой границы генератор не вызывается. Math.random и Action используют
одно состояние контекста; его перенос через снимок Math проверен отдельно.
Побочные эффекты valueOf сохраняются при исключениях, выход обработчика при
ошибке не меняется. Совпадение случайных последовательностей с историческими
плеерами и завершение стадий 1–9 не заявляются.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_exec_random_number` | function | [include/flashvm/execution.h:220](../include/flashvm/execution.h#L220) |
<!-- source-links:end -->
