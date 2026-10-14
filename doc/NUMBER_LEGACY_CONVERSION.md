# Number conversion / Преобразование Number — v0.193

Number(arg) и new Number(arg) в загруженных SWF используют существующий AVM1
numeric helper, как арифметические Action-коды. Отсутствующий аргумент даёт 0;
explicit undefined даёт 0 в SWF4–6 и NaN в SWF7–8. Строка "010" даёт 8 в
SWF6–8 и 10 в SWF4–5. Object-результат valueOf не вызывает дополнительный
host toString: результат 0 в SWF4, NaN в SWF5–8. Ошибки callback сохраняются.
Незагруженный C API сохраняет прежнее преобразование.

number_legacy_conversion проверяет function/constructor, версии, отсутствие
аргумента, undefined, строки, callback counts/errors и boxed value после GC.
Полные prototype layouts и отдельные версии функций/клипов остаются открытыми.

## English

Loaded Number(arg) and new Number(arg) now use the shared AVM1 numeric helper.
Missing arguments produce zero. Undefined produces zero in SWF4–6 and NaN in
SWF7–8; "010" produces eight in SWF6–8 and ten in SWF4–5. Object-valued valueOf
results do not invoke host string fallback: zero in SWF4, NaN in SWF5–8.
Callback errors propagate and unloaded host conversions remain unchanged.

The new fixture covers both entry points, version differences, missing and
undefined arguments, octal strings, callback counts/errors and boxed data after
traced GC. Reference: pinned Ruffle globals/number.rs function/constructor and
value.rs numeric coercion, commit 2372a4e148a67d68efd796f4477b3cddd4f880cd.
Prototype layouts, separate function/clip versions and all stages remain open.
