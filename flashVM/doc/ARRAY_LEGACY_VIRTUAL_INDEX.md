# Virtual native indices / Виртуальные индексы Array — v0.167

Загруженный native Array принимает addProperty для числового имени вместо
прежнего отказа. Descriptor сохраняет исходные indexed данные и flags, затем
старый indexed slot удаляется: обычный read вызывает getter, а loaded raw методы
читают retained data. Замена READ_ONLY/DONT_DELETE индекса допустима и сохраняет
flags. Незагруженный отказ для native индексов сохранён; length пока отклоняется.

array_legacy_virtual_index проверяет незагруженный режим и SWF4..8, исходную
строку a\0b, число, ссылку массива на себя, flags 4/2, getter 42 и raw slice с
исходными значениями. Трассируемый GC выполнен до и после channel transfer в
другой контекст: cycle, строки, descriptors и AVM1 getter сохраняются.
Readonly native setter dispatch, виртуальная native length, version masks,
argument semantics и все стадии остаются открытыми. ABI/wire не меняются.

## English

Loaded native Arrays accept addProperty for numeric names, preserving old indexed
data and flags in the script descriptor before removing the old indexed slot.
Ordinary reads invoke the getter; loaded raw methods read retained data.
Readonly/nondelete indexed replacement preserves flags. Unloaded native indexed
rejection is unchanged; native length replacement is still rejected.

Tests cover unloaded mode/SWF4..8, counted NUL strings, numbers, a self-reference,
flags 4/2, getter results and raw slice values. Traced GC runs before and after
channel transfer to another context, preserving the cycle, strings, descriptors
and AVM1 getter. Native readonly setter dispatch, virtual length, version masks,
argument semantics and all roadmap stages remain open. No ABI/wire changes.

## v0.168: readonly setter / Setter для readonly index

Loaded native Array теперь выполняет script setter при READ_ONLY индексных
данных, как Object; protected retained data не изменяются. Watcher по-прежнему
выполняется раньше, получает исходные raw данные и может преобразовать аргумент.
Проверен AVM1 setter с наблюдаемым свойством called и watcher, сохраняющий old
строку a\0b. Writable virtual index получает 81; readonly index сохраняет строку.
Setter/watcher повторно выполняются после traced GC и queue transfer в другом
контексте; receiver binding и temporary setter metadata восстановлены.
Readonly native length growth guard и виртуальная length остаются открытыми.

Loaded native Array script setters now execute for readonly indexed data,
matching Object behavior, while retained data remains protected. Watchers still
run first and receive original raw data. An AVM1 setter records a receiver marker;
a watcher retains the old counted string. Writable virtual data updates to 81;
readonly data remains unchanged. Both callbacks execute again after traced GC and
channel transfer in the destination context. Receiver binding and setter metadata
restore correctly. Readonly native length growth guards and virtual length remain
open. Unloaded behavior, ABI and wire formats are unchanged.
