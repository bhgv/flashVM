# Custom sort direction / Направление comparator — v0.158

В загруженном SWF Array.sort сначала сортирует по возрастанию, затем при
DESCENDING разворачивает весь результат, включая элементы с равными ключами.
Custom comparator возвращает порядок без инверсии; результат callback
приводится к числу через AVM1 bridge. Это соответствует полному пути
sort_compare_custom → sort_internal в pinned Ruffle 2372a4e.
Незагруженный режим сохраняет прежнюю инверсию отдельных сравнений.

array_legacy_sort_comparator проверяет незагруженный режим и SWF4..8:
native Array, borrowed Object, RETURNINDEXEDARRAY, разворот равных ключей,
сохранение receiver и отсутствие inherited getter вызова.
Проверки: AGG 326 и Cairo 325 тестов, переносимая сборка 305;
пять профильных проверок ASan/UBSan и 15 побайтовых SWF-эталонов.
Отказы запуска из-за прав исполнения повторно проверены успешно.
Точный порядок callback, undefined/holes, UNIQUESORT с callback,
masks/defaults, receiver/this и все стадии остаются открытыми. ABI/wire неизменны.

## English

Loaded Array.sort sorts ascending and then reverses the complete result for
DESCENDING, including equal keys. Custom comparison returns its direction
without inversion; callback results use AVM1 numeric conversion. This follows
the complete sort_compare_custom → sort_internal path in pinned Ruffle 2372a4e.
Unloaded sorting retains its existing per-comparison direction handling.
Tests cover unloaded mode/SWF4..8, native arrays, borrowed indexed returns,
equal-key reversal, unchanged receivers and absent inherited getter calls.
Validation: AGG 326, Cairo 325 and portable 305 tests, five focused
ASan/UBSan checks and 15 byte-exact SWF oracles. Execution-permission failures
passed after restoring executable bits. Full sanitizer run is still pending.
Exact callback order, undefined/holes, custom UNIQUESORT, masks/defaults,
receiver/this semantics and all roadmap stages remain open. No ABI/wire changes.
