# Loaded comparison order / Порядок сравнений — v0.163

Загруженные sort/sortOn используют quicksort с левым pivot и явным LIFO стеком,
как qsort в pinned Ruffle 2372a4e148a67d68efd796f4477b3cddd4f880cd.
Сначала обрабатывается левая часть. Все сравнения и обход partition учитываются
в общем instruction budget; квадратичный случай ограничивается этим бюджетом
и проверками worker cancellation. Незагруженный stable merge сохранён.
DESCENDING по-прежнему разворачивает готовый результат Array.sort.

array_legacy_sort_comparator фиксирует последовательность семи пар для
[3,1,4,2]: (3,1),(3,4),(3,2),(3,2),(3,4),(3,2),(2,1).
Та же последовательность проверена для ascending, descending и indexed return;
ошибка третьего сравнения не публикует reordered snapshot, roots/depth восстановлены.
Проверены исчерпание бюджета до callback и ограниченный отсортированный worst case.
Ключи builtin/fields ещё вычисляются заранее: точный порядок их coercion,
field lookup, custom UNIQUESORT и все стадии остаются открытыми.

## English

Loaded sort/sortOn now uses the reference left-pivot quicksort and explicit LIFO
partition stack, visiting left partitions first. Every comparison and partition
visit shares the instruction budget and worker cancellation checks, bounding the
quadratic worst case. Unloaded stable merging is preserved. Array.sort applies
DESCENDING by reversing the finished result.

Tests pin the seven comparison pairs for [3,1,4,2], in ascending, descending and
indexed-return modes. A third-comparison error leaves the reordered snapshot
uncommitted and restores roots/depth. Pre-callback budget exhaustion and a
bounded sorted worst case pass. Builtin/field keys are still prepared eagerly;
their exact coercion order, field lookup, custom UNIQUESORT and all stages remain
open. No ABI/wire changes. The reference algorithm is from pinned Ruffle qsort.
