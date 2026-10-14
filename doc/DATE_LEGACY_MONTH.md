# Date month setters / Установка месяца Date

v0.208: in loaded SWF4–8, `setMonth` and `setUTCMonth` replace a NaN
month with January. This also applies to omitted or undefined first operands
through the existing Date argument projection. Infinity still invalidates the
date; a NaN second (day) operand remains invalid. Unloaded host behavior is
unchanged. Conversion order, callback errors and the seven-operand projection
remain as documented in DATE_LEGACY_CONVERSION.md.

В загруженных SWF4–8 `setMonth` и `setUTCMonth` заменяют месяц NaN январём,
включая пропущенный/undefined первый аргумент. Бесконечный месяц и NaN дня
по-прежнему дают недействительную дату. Поведение незагруженного host-контекста
сохранено; порядок преобразования аргументов не меняется.

`tests/date_legacy_month.cpp` compares both methods with an explicit January
baseline in every supported SWF version and checks the stored timestamp.
The rule follows pinned Ruffle `core/src/avm1/globals/date.rs`, commit
2372a4e148a67d68efd796f4477b3cddd4f880cd, SET_MONTH branch. This source comparison
is not an independent Flash Player runtime oracle. Other calendar edge cases,
negative-time differences and full Date compatibility remain open.

Тест сравнивает оба метода с явным нулевым месяцем и проверяет сохранённое время.
Основание — указанная ветка исходников Ruffle; это не отдельный запуск Flash
Player. Остальные календарные особенности и полная совместимость Date открыты.

Validation / Проверка: GCC/G++ 13.3, C90/C++98, Linux AGG: 369 tests passed
(including 15 launch-permission retries). Linux NO_OS_PORTS: month, conversion
and fractional Date tests passed. This is not a non-Linux platform run or a
new sanitizer certification; roadmap stages remain open.

Additional validation / Дополнительная проверка: Linux Cairo Release, assertions
retained: all 369 tests passed (eight execution-permission retries). Both new
tests and Date conversion/fractional regressions passed in the heap/global
ASan + UBSan profile. Stack ASan and LeakSanitizer were disabled explicitly;
no full stack/leak or non-Linux runtime certification is claimed.
