# sortOn overloads / Перегрузки sortOn — v0.174

Загруженный sortOn сначала преобразует все имена полей, затем flags. Массив
полей использует собственные сохранённые элементы без getter/прототипа;
совпадающий по длине массив options читается так же, каждый элемент проходит
AVM1 numeric coercion и wrapping Int32. Общий примитивный option преобразуется
один раз. Неподходящий Object option или несовпадающий массив игнорируется.
Для одиночного примитивного имени учитывается только Number option.
Пустой массив полей возвращает this без сортировки. Object, который не Array,
выбирает обычное сравнение без преобразования имени/flags. Незагруженный режим
сохраняет прежние правила. Подготовка разделяет общий work budget с сортировкой.

array_legacy_sort_on_arguments проверяет SWF4..8, числовой/строковый/объектный
option, wrapping/fractional flags, виртуальные индексы со сохранёнными данными,
отсутствие getter calls, пустые поля, fallback и восстановление GC/depth.
Ограничение 64 поля сохранено. Полные signed native length semantics, native
callback GC, Unicode case mapping и все стадии роудмапа остаются открытыми.

## English

Loaded sortOn converts all field names before options. Field Arrays and matching
option Arrays read own retained data without getters/prototypes. Option entries
use AVM1 numeric coercion and wrapping Int32; a shared primitive option is coerced
once. Unsuitable Object options and mismatched Arrays are ignored. A single
primitive field accepts only a Number option. Empty field Arrays return this
without sorting. Non-Array Object fields select ordinary comparison without
name/option coercion. Unloaded semantics remain unchanged. Preparation shares
the sorting work budget.

Tests cover SWF4..8, numeric/string/Object options, wrapping/fractional flags,
retained virtual indices without getter calls, empty fields, fallback and restored
GC/depth state. The 64-field cap remains. Full signed native length semantics,
arbitrary native callback GC, Unicode case mapping and all stages remain open.
Reference: pinned Ruffle globals/array.rs sort_on.

Follow-up v0.523 / Продолжение: the loaded 64-field cap is removed; host policy
is retained. See [RU/EN contract](SORT_ON_MANY_FIELDS.md).
