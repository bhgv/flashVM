# Преобразования splice / Splice coercion order — v0.118

Начало `splice` преобразуется до чтения длины. Для собственного Array быстрый
путь выбирается после преобразования: getter, добавленный в прототип этим
callback, больше не пропускается. При переходе на общий путь передаётся уже
преобразованное число, поэтому исходный `valueOf` вызывается один раз.
Удаляемые элементы записываются как собственные поля результата, без вызова
унаследованного setter результата. Ошибка преобразования начала возвращается
до чтения длины; побочные эффекты callback не откатываются.

Правило порядка сверено с [pinned Ruffle splice](https://github.com/ruffle-rs/ruffle/blob/2372a4e148a67d68efd796f4477b3cddd4f880cd/core/src/avm1/globals/array.rs).
`array_splice_coercion` проверяет увеличение длины, порядок callback/length,
установку getter, единственный вызов `valueOf`, собственность удалённых полей,
невызванный setter и раннюю ошибку. Преобразование deleteCount может менять
прототип после выбора быстрого пути — этот случай пока открыт, как исторический
ToInt32, материализация дырок и ограничения общего изменяющего обхода. Барьер
нативного GC и все открытые стадии роудмапа сохраняются.

## English

Splice coerces start before capturing length and choosing the real-Array fast
path. Prototype getters installed by that coercion use the property-aware path,
which receives the converted number rather than invoking the original valueOf
twice. Removed values become own result fields without inherited setters.
Start-conversion errors propagate before length reads; callback effects remain.

The order was checked against the primary source above. The named test covers
length growth, evaluation order, getter installation, one conversion, own copies,
bypassed setters and early failure. Delete-count conversions that change the
prototype after path selection, historic ToInt32, hole materialization and
remaining generic traversal bounds still need work. Native GC barriers and all
roadmap acceptance gates remain open.
