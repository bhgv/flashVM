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
