# Date calendar remainder / Остаток времени при изменении даты

v0.212: calendar setters on valid loaded Dates (`setFullYear`, `setMonth`,
`setDate`, `setYear` and their available UTC counterparts) retain signed
millisecond remainder within a day in SWF4–7. SWF8 uses nonnegative remainder.
The final timestamp is floored, including negative fractions. Local variants
apply the remainder to local time, then use the existing calendar adapter.
Invalid-date recovery and unloaded host behavior retain their previous path.

Example: `new Date(-1).setUTCDate(31)` yields -86400001 in SWF4–7 and -1 in
SWF8. The same day argument can move the result back one day in the older
versions. This follows `Date::time_within_day` and the calendar setter branch
in pinned Ruffle commit 2372a4e148a67d68efd796f4477b3cddd4f880cd,
core/src/avm1/globals/date.rs. This source comparison does not establish full
Flash Player equivalence, invalid-date behavior or all timezone transitions.

В SWF4–7 календарные setters сохраняют знаковый остаток времени внутри суток;
SWF8 использует неотрицательный остаток. Итог округляется вниз. Для локальных
методов остаток вычисляется по локальному времени, затем работает существующий
адаптер календаря. Восстановление NaN-даты и host-контекст не меняются.
Например, повторная установка дня 31 в Date(-1) даёт -86400001 для SWF4–7,
но -1 для SWF8. Основание — указанные исходники; полная сверка Flash Player и
всех переходов часовых поясов остаётся открытой.

`date_legacy_calendar` checks seven positive/negative/fractional timestamps,
local and UTC full-year/month/day setters across SWF4–8. Expected results use
explicit day/remainder arithmetic and the current local offset. `setYear`
shares the implementation but is not separately covered by this new fixture.

Тест проверяет семь timestamps, год/месяц/день в местных и UTC методах для
SWF4–8. Ожидания рассчитаны через сутки/остаток и текущее смещение. setYear
использует общий код, но отдельного сценария в новом тесте нет.

Validation / Проверка: GCC/G++ 13.3, Linux C90/C++98: full AGG 372 and
Cairo Release 371 tests passed (four/five execution-permission retries).
Six selected Date tests passed in NO_OS_PORTS and heap/global ASan + UBSan.
Stack ASan and leak checks remain explicitly disabled. No real non-Linux
platform run or closed roadmap stage is claimed.
