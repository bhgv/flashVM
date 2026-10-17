# sortOn beyond 64 fields / sortOn с более чем 64 полями

v0.523 removes the artificial 64-field rejection from loaded SWF4–8 sortOn.
Fields and per-field flags already use dynamic vectors; preparation now grows
those vectors under the shared execution budget and ordinary allocation error
handling. Each loaded object field comparison also charges the same budget,
so many equal prefix fields cannot bypass cancellation/work accounting. The
unloaded host cap/policy is unchanged. No public ABI or graph-format changes.

В v0.523 снят искусственный предел 64 полей для загруженного SWF4–8. Поля и
флаги хранятся в динамических векторах; подготовка ограничивается общим
бюджетом исполнения и доступной памятью. Каждое сравниваемое поле также
учитывается в бюджете, включая длинные одинаковые префиксы. Host-политика,
публичный ABI и формат графа не меняются.

Primary comparison: Ruffle 2372a4e148a67d68efd796f4477b3cddd4f880cd,
avm1/globals/array.rs sort_on builds fields/options for the positive captured
Int32 count without a 64-entry limit. This is source comparison, not an
original Flash Player oracle. Actual maximum work/memory availability is not
a SWF compatibility promise; allocation errors need not roll back published
mutations. General Array/SWF semantics and all nine roadmap stages remain open.
Сравнение исходников не заменяет Flash Player; полная совместимость и все
девять этапов остаются открытыми. Отказы памяти не обещают откат изменений.

array_sort_on_many_fields checks 1/64/65/66/128/257 fields in SWF4–8 through
C API, interpreter and explicit/automatic JIT, plus graph roundtrips. All leading
fields compare equal; the final field and its option decide ascending/descending
order. Its roots variant checks 65 self-removing name conversions, late errors
and throws, native GC barriers and actual collection via a rooted direct callback.
Its allocation variant sweeps OOM/GC/retry, restoring pinned sorting inputs
after errors. Separate 32- and 100-step budgets interrupt preparation and
comparison before source writes, then successful retry checks quota cleanup.
The main regression fails with v0.522's artificial cap.

Проверки охватывают длинный список с решающим последним полем, флаги опций,
перенос графа, JIT, поздние ошибки, GC, отказы памяти и бюджет исполнения.

Validation / Проверено v0.523 on Linux x64 C++98: minimal 1080/1080,
AGG 1099/1099 and Cairo 1100/1100 passed without reruns. Seven targeted
ASan/UBSan cases passed. Fresh Unicorn instruction/ABI checks passed all 26
x86/ARM/Thumb/ARM64/RV32/RV64 cases; x64 is covered by native JIT execution.
Direct GNU readelf DT_NEEDED audit passed for 1089 Cairo ELF artifacts, finding
only libc, libgcc_s, libm and libstdc++. These emulator/Linux results do not
certify actual Windows/macOS/Android acceptance or transitive/dlopen dependencies.
Реальные сборки/запуски остальных ОС и полная совместимость остаются открытыми.
