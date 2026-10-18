# Setter replacement / Замена setter — v0.142

После успешного вызова собственного script setter запись разрешённого значения
повторно находит актуальное свойство receiver. Удалённый descriptor заменяется
обычными собственными данными; новый script descriptor получает retained data.
Повторного вызова setter нет. Текущий READ_ONLY после callback запрещает запись.
Counted строки копируются в контекст; старый и новый descriptor удерживаются
корнями GC. Унаследованный setter сохраняет правило отсутствия собственной записи.

object_setter_replacement проверяет удаление, замену виртуального свойства,
замену обычными данными и установку readonly внутри callback; counted NUL
значение сохраняется после GC и читается через raw-data slice. Подмена нативным
C accessor возвращает UNSUPPORTED после callback: скрытые данные такого accessor
пока не моделируются. Ошибка выделения после callback не откатывает его эффекты.
Array mutation set_data, readonly watcher order и все стадии остаются открытыми.

## English

After an own script setter succeeds, permitted storage resolves the receiver's
current property again. A deleted descriptor becomes ordinary own data; a new
script descriptor receives retained data, without invoking its setter again.
Current READ_ONLY suppresses storage. Counted strings are context-owned; GC roots
retain both descriptors. Inherited setters still do not create own receiver data.

object_setter_replacement covers deletion, virtual and ordinary replacement,
callback readonly changes, counted NUL values and raw-data slice after GC.
Replacement by a native C accessor returns UNSUPPORTED after the callback, whose
side effects are not rolled back on subsequent failure. Array mutation set_data,
readonly watcher order and every roadmap stage remain open.
