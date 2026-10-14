# Flash UCS2 case / Регистр Flash UCS2 — v0.180

Загруженные String.toUpperCase/toLowerCase и CASEINSENSITIVE сортировки используют
таблицу Flash по отдельным 16-bit units. Нет расширений ß/лигатур, контекстной
финальной сигмы или преобразования supplementary scalar: пары/одиночные
суррогаты сохраняются. Исторические mappings не исправляются по Unicode,
включая uppercase 03c2 → 03a2. Незагруженный host сохраняет Unicode 3.2 casing.
Pin: Ruffle 2372a4e148a67d68efd796f4477b3cddd4f880cd, wstr/src/tables.rs,
utils.rs и AVM1 globals/string.rs; данные/лицензия включены в external/flash_case.
scripts/generate_flash_case.py воспроизводит C++98 header без сети.

string_legacy_case проверяет SWF4..8/host: ß, İ, греческую сигму, лигатуру,
Deseret surrogate pair, NUL/одиночные суррогаты, post-GC retained output и
CASEINSENSITIVE+UNIQUESORT для İ/i. Существующие unloaded Unicode тесты сохранены.
Полнота property-name case/version semantics, все String API, независимая сверка
с оригинальным Flash Player и все стадии роудмапа остаются открытыми.

## English

Loaded String casing and CASEINSENSITIVE sorting map individual 16-bit units
through the pinned Flash table. There are no sharp-S/ligature expansions,
contextual final sigma or supplementary scalar casing. Paired/lone surrogates
remain unchanged. Historical mappings are retained, including uppercase
03c2 → 03a2. Unloaded host contexts retain Unicode 3.2 casing. Bundled Ruffle
tables/license and the generator reproduce a C++98 header without networking.

Tests cover SWF4..8/host sharp-S, dotted I, Greek sigma, ligature, Deseret pair,
NUL/lone surrogates, post-GC retained output and CASEINSENSITIVE+UNIQUESORT
for dotted I/i. Existing unloaded Unicode tests remain. Full property-name
case/version semantics, all String APIs, independent original Flash Player
validation and all roadmap stages remain open.
