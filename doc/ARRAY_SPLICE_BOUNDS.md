# Зафиксированные границы splice / Captured splice bounds — v0.119

Начало, длина и число удалений вычисляются один раз, в этом порядке, общим
внутренним помощником. Выбор быстрого или общего пути выполняется после обоих
преобразований. Getter прототипа, установленный при преобразовании deleteCount,
теперь учитывается без повторного чтения длины и повторных пользовательских
преобразований. Изменения исходной длины из deleteCount не меняют зафиксированные
границы. Быстрый путь не переносит индексы за пределами исходного диапазона,
если callback увеличил источник. Это предотвращает скрытые элементы вне
итоговой длины. Публичный ABI не изменён; структура границ не содержит GC-ссылок.

`array_splice_coercion` проверяет установку getter из обоих преобразований,
единственный вызов, рост и уменьшение источника, сохранение исходной длины,
собственные поля результата и отсутствие лишних индексов. Порядок сопоставлен
с [pinned primary source](https://github.com/ruffle-rs/ruffle/blob/2372a4e148a67d68efd796f4477b3cddd4f880cd/core/src/avm1/globals/array.rs).
Callback получения прототипа результата и watchers, меняющие источник во время
быстрой фиксации, требуют дополнительных проверок. Исторический ToInt32,
материализация дырок и ограничения общего обхода остаются открытыми, как все
стадии роудмапа. Побочные эффекты пользовательских callbacks не откатываются.

## English

One shared preparation captures start, length and deletion in that order.
Dispatch happens after both conversions, so deleteCount-installed prototype
getters are observed without repeated length reads or user conversions.
Subsequent length changes do not replace the captured bounds. The fast path
ignores source indices beyond the original interval after callback-driven growth,
preventing hidden slots outside the final length. Public ABI is unchanged; the
private captured structure has no managed edges.

The named test covers both conversions, getter installation, single calls,
source growth/shrinkage, captured lengths, own copies and excluded extra slots.
Order was checked against the pinned source above. Result-prototype callbacks,
source-mutating watchers during fast commits, historic ToInt32, hole creation
and generic traversal bounds remain open. Callback effects remain; no roadmap
stage is closed by this change.
