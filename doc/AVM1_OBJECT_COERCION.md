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
