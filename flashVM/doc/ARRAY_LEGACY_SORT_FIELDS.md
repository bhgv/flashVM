# Stored sortOn fields / Собственные поля sortOn — v0.165

Загруженный sortOn читает собственные данные поля при каждом сравнении, без
обхода prototype и вызова getter. Script virtual field использует retained data;
native Array length и собственные indexed slots читаются напрямую, lazy методы
материализуются без вызова. Операнды поля преобразуются через AVM1 bridge в
порядке left→right. Поля могут изменяться из toString: последующие сравнения
видят текущие данные. При не-Object элементах применяется builtin сравнение
с полными options первого поля, включая NUMERIC/DESCENDING.
Незагруженный field cache и API-контракт сохранены.

array_legacy_sort_coercion проверяет SWF4..8: counted NUL field, собственные
virtual данные без getter, точный порядок 14 conversions ascending/descending,
изменение поля первого record из callback и повторное чтение, ошибку coercion,
восстановление roots/depth, inherited getter без вызова и versioned missing data.
array_legacy_sort_keys проверяет numeric descending fallback для не-Object
элементов. Native display-property fallback, SWF case/version attribute masks,
sortOn argument overloads и все стадии остаются открытыми. Unknown C accessor
имеет undefined retained data; user getter не вызывается. ABI/wire неизменны.

## English

Loaded sortOn reads own stored field data on every comparison, without prototype
lookup or getters. Script virtual fields expose retained data; native Array length
and own indexed slots read directly, lazy method values materialize without calls.
Field values use left-before-right AVM1 conversion. Changes from toString are
visible to later comparisons. Non-Object elements fall back to builtin comparison
with the complete first-field options, including NUMERIC/DESCENDING.
Unloaded field caching and API behavior remain unchanged.

Tests cover SWF4..8, counted NUL names, virtual data without getters, pinned
ascending/descending coercion traces, callback field mutation and rereading,
conversion errors, restored roots/depth, inherited getter bypass, versioned
missing values and numeric descending primitive fallback. Native display-property
fallback, SWF case/version masks, argument overloads and all stages remain open.
Unknown C accessors retain undefined data and do not execute getters. No ABI/wire
changes. Reference: pinned Ruffle get_local_stored and sort_on_compare.
