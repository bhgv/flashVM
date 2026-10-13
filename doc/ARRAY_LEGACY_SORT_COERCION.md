# Loaded comparison coercion / Преобразования сравнения — v0.164

Для загруженного Array.sort встроенное сравнение преобразует значения при
каждом сравнении, сначала левое, затем правое, через AVM1 string bridge.
NUMERIC сравнивает числа напрямую, только когда оба операнда — Number.
Временный GC frame удерживает операнды и обе строки между callback; flags
CASEINSENSITIVE/DESCENDING применяются к результату по прежним правилам.
Незагруженные cached keys сохранены. sortOn field keys пока кешируются отдельно.

После custom comparator UNIQUESORT проверяет только соседние итоговые значения
встроенным сравнением, как полный sort_internal pinned Ruffle 2372a4e.
Он не вызывает custom comparator ещё раз и не является глобальным поиском
дубликатов в произвольном порядке callback. Отклонение возвращает 0 без
публикации reordered snapshot; эффекты callback не откатываются.

array_legacy_sort_coercion проверяет все 14 вызовов toString для эталонных
семи пар [3,1,4,2], ascending/DESCENDING, ошибку второго преобразования,
отсутствие reordered mutation и восстановление roots/depth в SWF4..8.
Comparator tests проверяют callback, объявляющий все разные значения равными,
и положительный callback для одинаковых чисел: UNIQUE использует builtin result.
Snapshot test теперь проверяет шесть conversion calls для трёх пар [2,1,3].
Native callback GC barrier, sortOn field lookup/coercion, полный Unicode case,
аргументы sort и все стадии остаются открытыми. ABI/wire не изменены.

## English

Loaded Array.sort converts builtin comparison operands on every comparison,
left before right, with the versioned AVM1 string bridge. NUMERIC compares directly
only when both values are Numbers. An intermediate GC frame retains operands and
converted strings across callbacks. Unloaded cached keys remain unchanged;
sortOn field keys are still prepared separately.

Custom UNIQUESORT checks adjacent final values with builtin comparison, following
the complete pinned sort_internal path, without invoking the custom callback
again. It is not a global duplicate search under arbitrary comparator ordering.
Rejection returns zero without committing reordered values; callback effects
remain. Tests pin fourteen toString calls for seven reference comparisons, both
directions, second-conversion errors and root/depth restoration in SWF4..8.
Custom equal/positive callbacks demonstrate independent builtin uniqueness.
Native callback GC barriers, sortOn fields, full Unicode case, argument handling
and all roadmap stages remain open. No ABI/wire changes.
