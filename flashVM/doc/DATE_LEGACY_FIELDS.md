# Date fractional fields / Поля дробных Date — v0.202

В загруженных SWF milliseconds getter возвращает целое число 0–999:
остаток локального/UTC timestamp сначала усекается к нулю, затем отрицательный
остаток нормализуется. Это отличается от простого округления дробного поля.
Hours getter учитывает историческое смещение +0.5 мс перед выделением часа;
Date text использует тот же час. Незагруженное поведение сохраняется.

## English

Loaded millisecond getters truncate the signed local/UTC remainder before
normalizing negative values, producing integer 0–999 and positive zero. Loaded
hour getters add 0.5 ms before the hour projection; local Date text uses the
same projection. Unloaded behavior is unchanged. Tests cover SWF4–8 exact UTC
negative/submillisecond/hour/day boundaries, integer local milliseconds and
local hour/text consistency. Timestamp data remains fractional and unchanged.

Reference: pinned Ruffle date.rs rem_euclid_i32, milliseconds, hours and to_string,
commit 2372a4e148a67d68efd796f4477b3cddd4f880cd. Its original Date SWF has header
17 and is excluded from the SWF≤8 execution corpus without modification; its
5.5-ms output records integer milliseconds 5 and timestamp 5.5. This is source-
based projection evidence, not actual Flash Player verification of all boundaries.
Calendar setter/version arithmetic, timezone fidelity and all stages remain open.

## Retained setter fields / Сохранённые поля setters

v0.211: loaded hour/minute/second/millisecond setters retain hours and
milliseconds using the same projections as getters, including fractional
hour-boundary rounding and negative-remainder normalization. This covers local
and UTC paths; unloaded host and calendar setters retain their existing path.
`date_legacy_retained` checks all four UTC setters in SWF4–8 against explicit
component arithmetic at negative fractions and positive hour/day boundaries.
The source reference is the same pinned Ruffle Date SET_HOURS..SET_MILLISECONDS
branch; this is not independent Flash Player confirmation.

Setters часов/минут/секунд/миллисекунд в загруженном SWF сохраняют часы и
миллисекунды по правилам getters, включая дробные границы и отрицательный
остаток. UTC-проверки охватывают все четыре метода для SWF4–8. Локальная ветка
использует тот же код; отдельной проверки всех часовых поясов это не означает.
Календарные setters и host-контекст сохраняют прежний путь.

v0.211 validation / Проверка: Linux GCC/G++ 13.3, C90/C++98: full AGG
371 and Cairo Release 370 tests passed (two/six launch-permission retries).
Five selected Date regressions passed in Linux NO_OS_PORTS and heap/global
ASan + UBSan; ASan stack and leak coverage remain explicitly disabled.
