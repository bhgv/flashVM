# Generic sort raw data / Данные общего sort — v0.157

Загруженный общий sort/sortOn использует own signed length и raw данные
элементов, без getter/prototype lookup. Snapshot заполняется всеми позициями,
включая собственные undefined вместо holes. Запись результата использует raw
own data без index setter/watcher, учитывая flags; method field getter/comparator
по-прежнему отдельный пользовательский код. Незагруженный путь не меняется.

[GC:] Snapshot удерживается allocation-free root через sort callbacks и запись
результата. Work accounting включает length coercion, input/copyback позиции и
AVM1 callbacks; полный учёт стоимости comparator merge ещё открыт.

array_legacy_sort_data проверяет SWF6..8, borrowed Object, virtual length/index,
prototype holes, readonly data, отсутствие index callbacks/watch и данные после
GC. Native fast sparse sort ещё имеет отдельное поведение holes. Callback timing,
все sort flags/return rules, native C accessor raw data, case/version и стадии
остаются открытыми. Generic cap 65536 остаётся; ошибка copyback может оставить
предыдущие изменения. ABI/wire не меняются.

## English

Loaded generic sort/sortOn reads own signed length and raw element data without
index/length getters or prototypes. Snapshot materializes every position,
including own undefined holes. Raw copyback skips index setters/watchers and
honors own flags. SortOn field getters/custom comparators remain user callbacks.
Unloaded behavior is unchanged. An allocation-free root retains the snapshot
through callbacks and copyback. Shared budget includes length coercion, input/
output positions and AVM1 callbacks; full merge comparison charging remains open.
SWF6..8 tests cover borrowed objects, virtual length/index, inherited holes,
readonly data, no index callbacks/watch and post-GC data. Native fast sparse
holes, callback timing, all flag/return rules, native C accessor data, case/version
and every stage stay open. Generic cap remains 65536; failed copyback may leave
prior changes. No public ABI/wire changes.
