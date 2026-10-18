# Date minute setters / Установка минут Date

v0.209: loaded `setMinutes`/`setUTCMinutes` truncate each minute, second and
millisecond component to signed 32-bit integer. NaN, infinity and values outside
[-2147483648, 2147483647] become -2147483648 rather than invalidating the date.
This is a range test before truncation, not ToInt32 modulo wraparound. Retained
seconds/milliseconds are projected too. Other setters and unloaded host
contexts keep their existing conversion rules. Argument callbacks are still
converted once, in order, by the shared Date projection before native mutation.

В загруженных SWF методы setMinutes/setUTCMinutes усекают минуты, секунды и
миллисекунды до знакового 32-битного целого. NaN, бесконечности и значения вне
диапазона дают -2147483648. Диапазон проверяется до усечения; переполнение не
оборачивается. Правило применяется также к сохранённым секундам/миллисекундам.
Остальные setters и незагруженный host-контекст сохраняют прежние правила.

Tests: date_legacy_minutes checks SWF4–8, every supplied component, positive and
negative fractions, both signed endpoints, values just outside the bounds,
NaN and both infinities. UTC expectations use explicit millisecond arithmetic;
retained fractional milliseconds, missing input and local NaN equivalence are
checked separately. Existing date_legacy_conversion covers conversion/error
ordering. Reference: pinned Ruffle commit
2372a4e148a67d68efd796f4477b3cddd4f880cd, core/src/avm1/globals/date.rs
SET_MINUTES branch and core/src/avm1/clamp.rs. This is source-based evidence;
full Flash Player/calendar/platform equivalence remains open.

Проверены перечисленные границы и компоненты для SWF4–8, арифметические UTC
результаты, пропущенный аргумент и сохранённая дробная миллисекунда. Основание —
исходники эталона; полной сверки Flash Player и платформ это не подтверждает.

Validation / Проверка: GCC/G++ 13.3 Linux AGG: 370 tests passed, with 14
launch-permission failures repaired and rerun. Linux NO_OS_PORTS: four Date
regressions passed. No new sanitizer or non-Linux runtime claim is made.

Additional validation / Дополнительная проверка: Linux Cairo Release, assertions
retained: all 369 tests passed (eight execution-permission retries). Both new
tests and Date conversion/fractional regressions passed in the heap/global
ASan + UBSan profile. Stack ASan and LeakSanitizer were disabled explicitly;
no full stack/leak or non-Linux runtime certification is claimed.
