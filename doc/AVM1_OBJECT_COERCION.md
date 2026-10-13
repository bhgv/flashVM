# AVM1 object coercion / Преобразование объектов

Loaded AVM1 numeric coercion (v0.97) invokes valueOf only; string coercion
(v0.98) invokes toString only and accepts a STRING result. Missing/noncallable
toString or a non-string result falls back to [type Object] or [type Function],
without invoking valueOf. A boxed String's stored payload bypasses override
lookup, preserving counted NUL/surrogate contents. Context recursion guards
recover after callback errors and nested coercion failure. Caller stack roots
and function-invocation guards retain operands and callback captures.

В загруженном AVM1 числовое преобразование не вызывает toString, а строковое
не вызывает valueOf. Нестроковый результат toString заменяется обозначением
типа. Преобразование boxed String берёт сохранённую строку и не вызывает
переопределённый метод. Изменения распространяются на обычный ToString, Trace
и преобразования имён членов/удаления/объявлений; embedding C API без SWF
метаданных сохраняет прежнюю семантику.

Regression avm1_object_string covers non-string/noncallable methods, counted
output, no numeric fallback, function spelling, boxed String bypass, missing
methods, errors and recovery after recursive coercion. avm1_object_number
covers the complementary numeric path. Full MovieClip/display-object paths,
Add2 default-hint behavior, standard constructors' version coercion, exception
fallback trace and historic global availability remain open. Stage 1/2 are not
complete. Primary comparison: [pinned Ruffle value implementation](https://github.com/ruffle-rs/ruffle/blob/2372a4e148a67d68efd796f4477b3cddd4f880cd/core/src/avm1/value.rs).

## v0.101 — Roots across primitive coercion

Add2/Less2/Equals2/Greater keep a temporary root vector for transformed operands
and derived string results. An earlier callback's string must remain live while
the next callback invokes script GC. StrictEquals has its separate non-coercing
path without this allocation. Add2 now converts right first; numeric relational
operators retain left-first conversion. Full default-hint object policy remains
open; this change fixes ordering and lifetimes independently.

Новый тест gc_avm1_primitive_coercion воспроизвёл ошибку сравнения после
настоящего gc.collect в другом valueOf. После исправления тридцать повторов
Add2/Less2/Greater сохраняют временные строки в traced режиме. Проверяется
и порядок нативных valueOf в Add2. Полная семантика hint/Date пока открыта.

## v0.102 — Default and numeric primitive policy

Loaded AVM1 now selects one primitive method: valueOf for ordinary objects and
numeric hints, toString for Date's default hint in SWF 6+, valueOf for Date in
SWF 5. Default-hint object results preserve the original object; numeric-hint
object results are retained for the operation's own nonprimitive rule. Add2
performs later string coercion only when a string operand selects concatenation.
Less2/Greater stop with false on a non-display object result, without coercing
the next operand or retrying toString. Numeric conversion uses the same helper.
Unloaded contexts retain their embedding primitive policy.

В тесте avm1_primitive_policy проверяются SWF 5–8, единственный выбранный
метод, Date hint, поздний строковый путь Add2 и остановка сравнения до второго
valueOf. Тест с GC продолжает проверять живые временные результаты. Полные
Equals2/player-specific NaN, MovieClip paths, standard constructors and function
version provenance remain open. Source comparison follows the pinned Ruffle
value implementation linked above; full external acceptance is still incomplete.
