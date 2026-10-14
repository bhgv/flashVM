# Numeric predicates / Числовые предикаты — v0.195

В загруженных SWF isNaN(arg) и isFinite(arg) преобразуют переданный аргумент
через общий AVM1 numeric helper. Explicit undefined в SWF4–6 становится нулём,
в SWF7–8 — NaN. Object-результат valueOf даёт ноль в SWF4 и NaN в SWF5–8,
без дополнительного host toString. Ошибки callback передаются вызывающему коду.
Без аргумента isNaN() по-прежнему true, isFinite() false во всех контекстах.
Незагруженный C API сохраняет прежние преобразования.

## English

Loaded isNaN(arg)/isFinite(arg) use AVM1 numeric coercion for supplied arguments.
Undefined becomes zero in SWF4–6 and NaN in SWF7–8. Object-valued valueOf becomes
zero in SWF4 and NaN in SWF5–8 without host string fallback. Errors propagate.
Missing arguments retain isNaN() == true and isFinite() == false; unloaded host
behavior remains. numeric_legacy_predicates covers both entry points, versions,
missing/undefined operands, callback counts/errors and restored root/depth state.

Reference: pinned Ruffle globals.rs is_nan/is_finite and value.rs numeric
coercion, commit 2372a4e148a67d68efd796f4477b3cddd4f880cd. Property metadata,
function/clip version contexts and all roadmap stages remain open.
