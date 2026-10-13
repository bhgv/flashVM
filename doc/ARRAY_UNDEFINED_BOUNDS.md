# Пустые аргументы splice / Undefined splice arguments — v0.117

`splice()` и `splice(undefined, ...)` возвращают undefined без чтения длины и
изменения массива. Явный undefined во втором аргументе также возвращает undefined
без вставки элементов; отрицательное число удаляемых элементов после текущего
целочисленного преобразования отклоняется так же. Пропущенный второй аргумент
по-прежнему удаляет остаток, а ноль и null допускают вставку без удаления.
Заимствованный метод учитывает эти правила до ограничения большого изменяемого
диапазона. `slice(start, undefined)` уже означал конец массива; добавлена
проверка этого существующего поведения, без заявления нового исправления.

Правила сверены с [pinned Ruffle array source](https://github.com/ruffle-rs/ruffle/blob/2372a4e148a67d68efd796f4477b3cddd4f880cd/core/src/avm1/globals/array.rs).
`array_undefined_bounds` проверяет собственный и заимствованный методы,
отсутствие чтения getter длины для отсутствующего начала, сохранение содержимого,
null/zero/omitted count и большой диапазон. Исторический ToInt32, порядок чтения
длины и преобразования объектного начала, изменения прототипа из преобразований
и ограничения остальных методов требуют дальнейшей проверки. Стадии открыты.

## English

Missing/undefined splice starts return undefined without reading length or
mutating. Explicit undefined deletion counts and negative counts after the
current integer conversion return undefined without insertion. Omitted counts
remove the tail; zero and null allow insertion without deletion. Generic size
limits are checked after these rejected-count cases. A regression test confirms
the already implemented slice(start, undefined) end-of-array rule.

Rules were compared with the pinned primary source linked above. The named test
covers real/borrowed receivers, observable length accessors, preserved contents,
valid null/zero/omitted counts and huge rejected ranges. Historic ToInt32,
object-start versus length evaluation order, coercion-driven prototype changes
and other Array restrictions remain open, as do all roadmap stages.
