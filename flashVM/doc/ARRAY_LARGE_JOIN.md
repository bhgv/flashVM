# Большие Array.join / Large Array.join — v0.115

Сняты ограничения 65 536 элементов у собственного и заимствованного `join`.
Размер фиксируется до преобразования разделителя; последующие чтения учитывают
изменения прототипа из этого преобразования и наследуемые getter. Отверстия,
null и undefined дают пустые части; разделитель сохраняет встроенный NUL.
Защита от рекурсивного join остаётся и снимается при ошибках.

Обход элементов и записываемые байты расходуют общий `instruction_limit`
с вложенными callbacks. При нулевом лимите фиксированного порога длины нет,
но память для результата должна быть доступна. Это не полный лимит CPU:
преобразования нативных строк имеют собственную стоимость. Барьер нативного GC
сохраняется. Остальные ограничения Array и стадии роудмапа остаются открытыми.
Тесты `array_large_join` и `array_join_allocation` проверяют большой диапазон,
counted-разделитель, изменения прототипа, бюджет и каждый отказ выделения
памяти на компактном примере до первого успешного выполнения.

## English

Own and borrowed join no longer reject lengths above 65,536. Length is captured
before separator conversion. Live reads see inherited getters installed by that
conversion; holes/null/undefined produce empty parts, and counted separators
retain embedded NUL. Recursive join guards unwind on errors.

Slot visits and output bytes share instruction_limit with nested callbacks.
A zero limit imposes no fixed length cap; result allocation can still fail.
This is not a complete CPU bound for native string conversions. Native GC barriers,
other Array limitations and all roadmap acceptance gates remain unchanged/open.
The tests above cover large ranges, counted separators, live prototype changes,
work limits and every allocation failure through success on a compact fixture.
