# Native sort keys / Ключи сортировки — v0.156

Нативный быстрый sort/sortOn в загруженном контексте больше не вызывает watcher
при commit. Примитивные ключи sort и field keys sortOn используют AVM1 string
bridge: Boolean в SWF4 даёт "0"/"1", отсутствующее поле sortOn до SWF7 даёт
пустую строку. Незагруженные conversion/watch правила сохраняются.

array_legacy_sort_keys проверяет незагруженный режим и SWF4..8, fast watcher
bypass, смешанные Boolean/string ключи и missing field в sortOn. Существующие
sort flags/comparator/GC проверки сохраняются. Общий путь ещё читает через
get/prototype и пишет через ordinary API; callback ordering, masks/defaults,
holes/size limits, case/version и все стадии остаются открытыми. Precomputed
ключи не подтверждают полную историческую последовательность toString/getter.

## English

Loaded native fast sort/sortOn bypasses watchers at commit. Primitive sort keys
and sortOn field keys use the AVM1 string bridge: SWF4 booleans become "0"/"1",
and missing fields before SWF7 become empty strings. Unloaded conversion/watch
behavior stays unchanged. Tests cover unloaded mode/SWF4..8, watcher bypass,
mixed Boolean/string ordering and missing sortOn fields. Existing flags,
comparators and GC checks remain required. Generic property/prototype reads and
ordinary writes, callback order, masks/defaults, holes/size, case/version and all
stages stay open. Precomputed keys do not certify full historical callback order.
