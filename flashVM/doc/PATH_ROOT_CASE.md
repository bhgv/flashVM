# Exact _root path keyword / Точное ключевое слово _root в пути

v0.518: only the exact first-token spelling _root selects the display root in
SWF5–8 target paths. _ROOT, _Root and suffix names use ordinary child/property
resolution, including SWF5/6 case folding of stored property names, accessors
and exceptions. Their stored values can shadow the display root. Later path
tokens remain ordinary properties. SWF4 retains its restricted path syntax;
unloaded host behavior is unchanged.

В v0.518 только точное _root в первом сегменте выбирает корень сцены SWF5–8.
_ROOT, _Root и имена с суффиксом разрешаются как обычные дочерние объекты/свойства.
Сохраняются регистр свойств SWF5/6, getter и ошибки; свойство может затенить корень.
Последующие сегменты обычные. Правила SWF4 и host без загруженного SWF сохранены.

The shared resolver serves qualified GetVariable, SetVariable and CallFunction.
The latter retains the resolved member owner as its receiver. This fixes the
former unconditional root selection for mixed-case names in SWF5/6 With scopes.
Общий обработчик обслуживает чтение, запись и вызов; вызов сохраняет правильного
получателя. Раньше смешанный регистр внутри With в SWF5/6 ошибочно выбирал корень.

Primary comparison: Ruffle 2372a4e148a67d68efd796f4477b3cddd4f880cd,
avm1/activation.rs resolve_target_path tests name == b"_root" separately from
ordinary case-sensitive/folded property resolution. This source comparison is
not an original Flash Player oracle.
Сравнение исходников не заменяет проверку оригинальным Flash Player.

path_root_case covers host/SWF4–8, exact/mixed/suffix spellings, dot/colon/slash
and later tokens, read/write/call receiver selection, C execution helpers,
interpreter, explicit/automatic JIT and graph roundtrips with cycles and pure
AVM1 functions. path_root_case_roots checks skipped versus executed getters,
self-removal, detached captures, collecting callbacks, throws and errors.
path_root_case_allocation sweeps allocation failures, unchanged output on OOM,
GC and successful retry, including counted NUL suffixes. The regression is
reproduced on v0.517 before changing the shared resolver.

Проверки включают C API, интерпретатор, JIT, перенос циклического графа, GC,
исключения и отказы выделения памяти. Формат графа и публичный ABI не меняются.
General display-path compatibility, complete SWF semantics and all nine roadmap
stages remain open / полная совместимость и все девять этапов остаются открытыми.

Validation / Проверено v0.518 on Linux x64 C++98: minimal 1067/1067,
AGG 1086/1086 and Cairo 1087/1087 passed, including successful reruns after
restoring executable permissions for 1/2/3 binaries respectively. Six targeted
ASan/UBSan cases passed. The corrected matrix linked to the v0.517 resolver
reproduces SWF5 _ROOT:marker selecting 17 instead of the shadow object's 91.
Fresh Unicorn checks passed all 26 x86/ARM/Thumb/ARM64/RV32/RV64 instruction
and ABI cases; host x64 is exercised by native JIT tests. These are emulator
checks, not foreign operating-system acceptance. Direct DT_NEEDED audit passed
for 1076 Cairo ELF artifacts, finding only libc, libgcc_s, libm and libstdc++.
It does not inspect transitive/dynamic dependencies or certify other OSes.
Все перечисленные проверки успешны; реальная приёмка Windows/macOS/Android
остаётся открытой.
