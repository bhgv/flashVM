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
