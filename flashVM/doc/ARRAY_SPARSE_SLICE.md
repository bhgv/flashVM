# Разреженные срезы Array / Sparse Array slices — v0.114

`Array.slice` и заимствованный `slice` больше не отклоняют результат длиннее
65 536 элементов. Длина источника фиксируется до преобразования границ;
результат сохраняет отверстия и выбранную длину вплоть до uint32 maximum.
Большие разреженные диапазоны обходят существующие собственные и наследуемые
индексы в возрастающем порядке. После getter список будущих индексов собирается
заново: добавленные индексы учитываются, удалённые пропускаются. Уже пройденные
отверстия не перечитываются. Плотные диапазоны обходятся последовательно.
Преобразование границ может менять прототип до начала чтения элементов.
Копии создают собственные поля, минуя setter прототипа результата; числовые
имена со встроенным NUL не принимаются за индекс по усечённому префиксу.

`instruction_limit` теперь также ограничивает работу обхода `slice`; вложенные
AVM1 callbacks используют тот же счётчик. Проверка остановки worker периодическая.
Это не полный лимит CPU для всех стандартных методов. Ошибки callbacks и памяти
не публикуют успешный результат; побочные эффекты пользовательских callbacks
не откатываются. Барьер нативных вызовов GC сохраняется. Другие методы Array
пока имеют собственные ограничения; полная историческая семантика прототипа
результата, Unicode-регистр и остальные пункты стадии 1 остаются открытыми.

Тесты: `array_sparse_slice`, `array_slice_allocation`, `array_counted_ranges`,
`array_generic`. Проверяются диапазон uint32, getter-мутации, преобразование
границ, общий бюджет callbacks и отказы каждого выделения памяти до успеха.

## English

Real and borrowed `Array.slice` no longer reject result ranges over 65,536
slots. Length is captured before endpoint coercion; holes and the selected
uint32 length are retained. Sparse traversal sorts existing own/inherited
indices and refreshes future candidates after getters. Dense ranges use ordinary
sequential reads. Endpoint conversion may install inherited getters. Copied
slots are own data properties without inherited destination setter calls;
embedded-NUL names are not treated as numeric prefixes.

Native traversal and nested AVM1 callbacks share `instruction_limit`, with
periodic worker cancellation checks. This is not a complete CPU bound for every
standard method. Allocation/callback errors publish no successful result and
restore execution nesting, while user callback side effects remain. Existing
native GC barriers remain. Other Array method bounds, historical result
prototype semantics and all remaining stage-1 acceptance gates remain open.

The tests above cover uint32 ranges, live getter mutations, coercion changes,
shared callback budgets and injected allocation failures through success.

## Follow-up v0.131 / Последующее уточнение

Loaded SWF execution now uses historical own-data, signed Int32 and explicit
undefined-slot rules; see [ARRAY_LEGACY_SLICE.md](ARRAY_LEGACY_SLICE.md).
Описанный здесь sparse/property-aware режим сохраняется для незагруженного C-контекста.
