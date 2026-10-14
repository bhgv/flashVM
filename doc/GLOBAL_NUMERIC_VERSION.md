# Numeric globals / Числовые глобальные свойства — v0.178

NaN/Infinity читаются через versioned native getters: загруженный SWF4 получает
undefined, SWF5..8 — IEEE значения. Записи в загруженном режиме игнорируются.
Незагруженный контекст сохраняет изменяемые host значения, включая Object/NUL
строки. Getter использует текущую версию контекста, поэтому установка standard
до загрузки SWF также работает. Оба maps публикуются только после успешной
подготовки. Захваченные host значения имеют GC roots и сериализуемые native
callbacks с новыми стабильными numeric IDs 158..160; старые IDs сохранены.

global_numeric_version проверяет до/после загрузки, SWF4..8, незагруженные
присваивания, игнорирование loaded writes, traced GC, канал/GC получателя и
изменение результата после загрузки SWF в получателе. Оригинальный equals_swf4
теперь совпадает с output.txt побайтно (4049 bytes), без изменения SWF/ожиданий.
Версионная доступность/регистр прочих глобальных свойств, attribute masks,
отдельные версии функций/клипов и все стадии роудмапа остаются открытыми.

## English

Versioned native getters return undefined for loaded SWF4 and IEEE NaN/infinity
for SWF5..8. Loaded writes are ignored. Unloaded contexts retain mutable host
values, including Objects and counted NUL strings. Reading the current context
version supports standard installation before SWF loading. Both maps are
published after successful staging. Captured host values are GC-rooted and
transportable with new stable numeric callback IDs 158..160; old IDs are unchanged.

Tests cover installation before/after loading, SWF4..8, unloaded assignments,
ignored loaded writes, traced GC, channel transport/destination GC and version
switching when the destination loads a SWF. The original equals_swf4 now matches
its 4049-byte output exactly without changing the SWF or expected output. Other
global availability/case rules, attribute masks, separate function/clip versions
and all roadmap stages remain open. Reference: pinned globals.rs get_nan and
get_infinity.
