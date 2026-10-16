# Атрибуты отдельных методов / Per-method prototype activation

RU: v0.486 отделяет атрибуты методов Number/Date/String/Array от решения о
замене внутреннего host-layout прототипа. Изменение или удаление valueOf,
getTime, charAt либо push больше не оставляет остальные нетронутые методы
с host-флагами. Каждый собственный native callback с исходными флагами
DONT_ENUM получает DONT_ENUM|DONT_DELETE независимо от других методов.
Изменённые data/accessor-поля, удалённые методы и явно изменённые флаги
сохраняются. Getter не выполняется при подготовке. Прежние ограничения
преобразования payload/length и удаления host-only Array.toLocaleString
сохраняются. Ошибка выделения памяти не публикует частичные таблицы флагов.

EN: Per-method metadata preparation no longer depends on the anchor method or
payload guard used for optional prototype layout conversion. Preparation still
requires the corresponding native resource kind, avoiding writes to unrelated
replacement prototype tables. Number/Date/Array stage independent attribute
owners; String stages its table before selecting optional layout conversion.
All allocating preparation precedes commit. Existing custom payload/layout
preservation and reload behavior remain unchanged. The host-only Array locale
alias keeps its previous transition rules.

Tests: prototype_attribute_activation crosses four native prototype classes,
all SWF4–8 versions, six anchor shapes and six peer shapes (default, data,
deleted, accessor, explicit flags 0/7). It checks C/interpreter/explicit/automatic
JIT writes, nondeletion, graph transport without destination standard install,
GC, getter suppression and reload. Custom native accessors are omitted from
transport cases because arbitrary embedding callbacks have no stable codec ID.
prototype_attribute_allocation sweeps first-load new/new[] failures after three
anchor replacements, verifies old flags, then GC/retry and restored scopes.
C malloc injection and unrestricted native callback GC are not newly covered.

Reference: standard declarations in pinned Ruffle
[Number](https://github.com/ruffle-rs/ruffle/blob/2372a4e148a67d68efd796f4477b3cddd4f880cd/core/src/avm1/globals/number.rs),
[Date](https://github.com/ruffle-rs/ruffle/blob/2372a4e148a67d68efd796f4477b3cddd4f880cd/core/src/avm1/globals/date.rs),
[String](https://github.com/ruffle-rs/ruffle/blob/2372a4e148a67d68efd796f4477b3cddd4f880cd/core/src/avm1/globals/string.rs),
and [Array](https://github.com/ruffle-rs/ruffle/blob/2372a4e148a67d68efd796f4477b3cddd4f880cd/core/src/avm1/globals/array.rs).
Host-before-load activation is FlashVM embedding behavior; this is not an
original Flash Player oracle pass. All nine roadmap stages remain open.
