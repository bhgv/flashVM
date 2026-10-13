# Наследуемые элементы concat / Inherited concat elements — v0.116

`Array.concat` теперь читает собственные и наследуемые индексы всех разворачиваемых
массивов, включая аргументы. Большие разреженные диапазоны используют возрастающий
обход с обновлением будущих индексов после getter. Длина каждого источника
фиксируется при его очереди; ранее выполненные getter могут изменить следующие
источники. Результат имеет собственные поля, без вызовов setter его прототипа.
Переполнение общей uint32 длины отклоняется.

Обычный объект — получатель заимствованного метода — разворачивается по длине;
обычный объект среди аргументов остаётся единичным значением. Правило сверено
с `concat` в исходниках Ruffle, ревизия
`2372a4e148a67d68efd796f4477b3cddd4f880cd`,
[primary source](https://github.com/ruffle-rs/ruffle/blob/2372a4e148a67d68efd796f4477b3cddd4f880cd/core/src/avm1/globals/array.rs).
Это проверка конкретного правила, а не полная сверка исторических SWF.

Обход разделяет лимит работы с вложенными callbacks; ошибки памяти восстанавливают
глубину выполнения и не публикуют успешного результата. Побочные эффекты getter
не откатываются, барьер нативного GC остаётся. Исторические signed-length правила,
материализация дырок в undefined, прототип результата и полный перечень объектов
для заимствования требуют дальнейшей проверки; все стадии остаются открытыми.
Тесты: `array_concat_sparse`, `array_concat_allocation`, `array_generic`.

## English

Concat reads own/inherited indices in both its receiver and array arguments.
Large sparse sources use ordered candidate traversal refreshed after getters;
each source length is read at its turn. Copies create own data slots, bypassing
inherited destination setters, and uint32 length overflow is rejected.
A borrowed plain-object receiver spreads by length; a plain-object argument
remains one element. This rule was checked against the pinned primary source
linked above; it is not a complete historic SWF oracle comparison.

Traversal shares work limits with callbacks. Allocation errors restore nesting
and publish no successful result; getter side effects remain, as do native GC
barriers. Signed-length history, hole materialization, result prototype and full
receiver classification still require acceptance. All roadmap stages remain open.
The tests above cover live array-argument getters, large sparse ranges, plain
receiver/argument distinction, budgets and exhaustive allocation-failure retries.
