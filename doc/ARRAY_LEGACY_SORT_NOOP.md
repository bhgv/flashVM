# Sort no-op dispatch / Ранний выход сортировки — v0.175

Загруженный sort с неподходящим первым примитивом и sortOn без аргументов
возвращают undefined до проверки длины и создания borrowed snapshot. Пустой
массив полей (включая отрицательную signed длину) возвращает this на том же
этапе. Свойства receiver не читаются и не переписываются. Это исключает
побочные эффекты копирования и ошибку бюджета/ограничения массива для no-op.
Незагруженные правила сохранены.

array_legacy_sort_noop проверяет SWF4..8, native длину 100000, borrowed Object
с getter длины, бюджет 1, отсутствующий индекс и сохранение исходного элемента.
array_legacy_sort_on_coercion проверяет последовательность field0, field1,
option0, option1, изменение option1 из field0, ошибки в field1/option1,
отсутствие sorted copyback при ошибке, сохранение callback side effects и
восстановление root/depth/setter state.

## English

Loaded sort with an unsuitable primitive first argument and argument-free
sortOn return undefined before receiver length checks or borrowed snapshots.
Empty field Arrays, including a negative signed length, return this at the same
stage. Receiver data is neither read nor rewritten. No-op calls therefore avoid
copying side effects and budget/array-cap errors. Unloaded rules are preserved.

Tests cover SWF4..8, a 100000-length native Array, borrowed Object length getter,
budget 1, missing indices and unchanged data. Coercion tests pin field0, field1,
option0, option1 order; field0 changes option1, errors in field1/option1 avoid
sorted copyback, retain callback changes and restore root/depth/setter state.
Full Array/AVM1 semantics, arbitrary native callback GC and all stages remain
open. Reference: pinned globals/array.rs argument dispatch before sort_internal.
