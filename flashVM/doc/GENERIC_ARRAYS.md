# Generic Array methods and registered classes — v0.17

## English

Array methods can be borrowed through Function.call/apply by ordinary receivers
with a length property. push/pop/shift/unshift/reverse/slice/splice/join/concat/
sort/sortOn now have a property-based path. Numeric length is converted with
ToUint32. Holes stay absent, inherited numeric entries/accessors are read, writes
use the actual receiver's property API, and property flags/watch handlers apply.
Real Arrays with inherited numeric slots use this path too. reverse creates own
destination properties while an inherited source remains on its prototype.
Recursive generic join returns an empty segment; counted strings retain NUL.
Borrowed concat treats a non-Array receiver as one item instead of spreading it.
Sort takes a snapshot, retains stable order and preview/unique behavior, then
writes through the original receiver. Generic receiver writes are incremental:
callback/assignment failures can retain earlier side effects. This differs from
fast own-slot Array snapshot commits; neither mode rolls back callback effects.
Whole-length generic operations currently reject lengths above 65536; push/pop
can operate on larger lengths within uint32 bounds. This is a remaining limit,
not full Array compatibility. Numeric accessor deletion now clears the descriptor.

Object.registerClass(symbolId, constructor) returns a Boolean, replaces a binding
or unregisters it for null/undefined. Invalid constructors return false without
removing a previous binding. The static method retains a private traced registry,
so constructor graphs survive GC and travel with Object and its captured methods.
[fvm_registered_class](../include/flashvm/standard.h#L38) resolves a borrowed constructor through the current Object
class global and returns NOT_FOUND if absent. Imported registries are independent
of the sender; importing the Object class does not install it as a global until
the host/script explicitly binds it. Integrating registrations with SWF
ExportAssets and attachMovieClip is still a stage-3 task.

Acceptance scenarios: array_generic.cpp covers borrowed methods, inherited getters,
prototype holes, recursive join, watched writes, numeric descriptor deletion and
calling an imported method without standard installation. object_register_class.c
covers replacement validation, GC retention, unregistering and independent imports.
No full stage-1 completion is claimed; see COMPATIBILITY_STATUS.md.

## Русский

Методы Array можно заимствовать через Function.call/apply для обычных объектов
с length. Добавлен путь через свойства для перечисленных выше методов: он читает
наследуемые индексы и getters, сохраняет разреженность и записывает значения через
реальный объект с учётом флагов и watch. Настоящие Array с наследуемыми индексами
также используют этот путь. Циклический generic join завершается, NUL сохраняется.
Concat обычного объекта помещает сам объект первым элементом, не раскрывая его.
Сортировка читает снимок и записывает результат через исходный объект. При ошибке
сохраняются предыдущие записи и действия обработчиков. Полные проходы ограничены
65536 позициями; push/pop допускают большие длины в пределах uint32. Полная
совместимость Array этим этапом не подтверждается. Удаление числового accessor
теперь удаляет и его дескриптор.

Object.registerClass создаёт/заменяет привязку символа к функции, возвращает bool;
null/undefined снимают привязку. Некорректная функция не удаляет прежнюю запись.
Хранилище отслеживается GC и переносится с Object/захваченным методом. Получатель
имеет отдельное хранилище. C API [fvm_registered_class](../include/flashvm/standard.h#L38) ищет класс через текущий
глобальный Object; импорт не устанавливает глобальные имена автоматически.
Связь с ExportAssets/attachMovieClip предстоит реализовать на этапе 3.

Тесты и проверяемые сценарии перечислены выше. Остальные ограничения этапа 1
сохраняются в реестре совместимости.

## Validation / Проверки

Linux strict C90/C++98: headless 167, static AGG 177, static Cairo 177, minimal
profile 158, standalone ASan/UBSan 164 passed. Headless launch-permission failures
passed after executable restoration and rerun. Sanitizer exclusions are unchanged:
--no-asan-stack, no LeakSanitizer, container_exceptions.cpp UBSan only, no SQLite.
These results do not certify full stack/leak coverage or other-platform execution.
Логи и профили остаются вне Git; завершение этапа не объявляется.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_registered_class` | function | [include/flashvm/standard.h:38](../include/flashvm/standard.h#L38) |
<!-- source-links:end -->
