# Удалённые элементы splice / Loaded splice removed slots — v0.134

В загруженном SWF removed Array содержит собственное значение для каждой
удалённой позиции, включая undefined для дырки/accessor/наследуемого элемента.
Property-aware путь больше не вызывает getter при формировании removed.
Обычный быстрый путь заполняет отсутствующие собственные индексы. Материализация
расходует общий бюджет AVM1; при его исчерпании до мутации source не меняется.
Бюджет включает вложенные преобразования границ в рамках того же вызова.

Созданный removed/копируемый результат удерживается allocation-free корнем
[FVM_ObjectRootScope](../src/core/object_root_scope.hpp#L8) до завершения native-метода. Корень не меняет calling-owner
и восстанавливается при возвратах и исключениях. Используется также для общих
slice/splice результатов. Корень защищает graph/строки при поздних callback.

array_legacy_splice_removed проверяет SWF6..8 быстрый и property-aware пути,
нулевое число getter-вызовов для removed, собственные undefined, строку после
GC и неизменность source при исчерпании бюджета. Первоначальный тест потерял
свой незакреплённый host-массив методов после GC; fixture теперь явно pin-ит
все host-объекты, используемые после сборки.
array_legacy_splice_allocation перебирает отказы выделения в быстром загруженном
пути: ошибка оставляет source неизменным, цепочку корней/глубину восстановленной;
GC выполняется после отказов и до проверки успешного закреплённого removed.
Тест не заявляет фактический сбор внутри watcher: native GC barrier сохраняется.

Мутации source всё ещё имеют property-aware move/write; исторический set_data,
signed/own-data длина и атомарность generic mutation требуют следующей работы.
Большие generic mutating обходы ограничены. Все стадии роудмапа остаются открытыми.

## English

Loaded splice removed results define every position, including own undefined
for missing/accessor/inherited source elements. Generic removed copying bypasses
getters; the fast path fills missing own slots. Materialization charges shared
AVM1 work. Budget exhaustion before source mutation preserves the receiver.
The scope also shares endpoint conversion work with nested execution.

An allocation-free object root scope retains the removed/copy result graph until
native-method completion, restoring the previous chain on errors/exceptions
without changing calling-owner metadata. Shared slice/splice results use it.
The new tests check SWF6..8 fast/generic removed copying, no getter invocation,
own undefined, strings after GC, budget preservation and exhaustive fast-path
allocation failures. Successful results are pinned before real collection;
failed attempts collect after verifying unchanged source/root/depth state.
The initial fixture needed a host-method Array pin across its own collection.
No actual collection inside native watchers is claimed; the native barrier stays.

Historical source set_data/movement, signed own-data length and generic mutation
atomicity remain open. Large generic mutations stay restricted. All stages remain
open.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_ObjectRootScope` | class | [src/core/object_root_scope.hpp:8](../src/core/object_root_scope.hpp#L8) |
<!-- source-links:end -->
