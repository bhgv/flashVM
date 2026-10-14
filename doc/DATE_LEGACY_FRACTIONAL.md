# Date fractions and text / Дробные timestamp и текст — v0.201

Загруженный single-timestamp конструктор сохраняет дробную часть конечных
значений в поддерживаемом диапазоне ±8640000000000000 мс. setTime после
проверки диапазона округляет вниз, включая отрицательные дроби. Host C API
сохраняет truncation. Loaded Date text выводит день месяца без ведущего нуля;
host spelling сохраняется. Native IDs, graph/API formats не изменились.

## English

Loaded single-timestamp construction retains finite bounded fractional data;
setTime floors after the existing range check, including negative fractions.
Unloaded host truncation is unchanged. Loaded Date text uses an unpadded day
while host spelling stays padded. Callback IDs and graph/API formats remain.

Tests cover SWF4–8/host positive/negative fractions, distinct constructor/setter
results, native Date graph transfer with post-GC retained fractional data,
local day spelling, setter NaN/Infinity/range guards and rejection of typed
CRC-valid Date records with infinity/out-of-range data, plus encoder rejection
of wrong state types.
The decoder now accepts bounded fractional Date states without changing layout. Reference: pinned Ruffle
date.rs constructor, Date.clip and Date.to_string, commit
2372a4e148a67d68efd796f4477b3cddd4f880cd. The existing constructor range/nonfinite
guard is retained; unbounded raw timestamp compatibility requires a calendar
engine audit. Other calendar fields/version arithmetic, locale fidelity, actual
Flash Player comparison for these Date cases and all roadmap stages remain open.
