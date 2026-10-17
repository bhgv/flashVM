# Property watches and enumeration — v0.14

Object.watch(name, handler, userData) registers a first-class function. It receives
(name, oldValue, requestedValue, userData) with the watched object as this; its
return value becomes the stored value. unwatch(name) returns whether a handler
was removed. Deletion keeps the watch for later assignment. Reentrant assignment
to the same watch skips that handler; other watches still run. Callback exceptions
propagate and prevent the pending assignment. Callback side effects are retained.
Read-only slots do not invoke handlers. Accessor properties and MovieClip's native
transform fields cannot be watched. Array indexed writes participate, including
writes staged by its mutation methods; deleting holes does not invoke a watch.

GC roots the owner, descriptor, incoming value, old value and user data during
callbacks, even when the handler removes itself and requests collection. Graph
format 3 preserves watches across queue/file/TCP transfer without installing the
standard library at the receiver. Version 1/2 readers are supported; old readers
cannot consume version 3. Capability objects with attached watches are rejected
as unsupported instead of silently dropping the watch; resource annotations
remain an outstanding transport task.

[fvm_object_enumerate](../include/flashvm/property.h#L41) produces an Array snapshot of enumerable own properties,
optionally including prototypes. Nonenumerable own names shadow inherited names;
getters are not invoked. Numeric slots precede named entries. AVM1 Enumerate and
Enumerate2 use this snapshot with a null stack sentinel. Default native meta
iteration skips DONT_ENUM slots. Ordering is deterministic, not a promise of
identical historical Flash hash ordering.

Function.call/apply capture boxing constructors for use after graph import into a
context without standard globals. TextFormat is a transferable first-class
constructor. TextField remains nonconstructible through new. AVM1 variable lookup
respects explicit undefined lexical bindings and resolves receiver properties;
negative numeric member keys become names, while native table numeric slots keep
their indexed API semantics.

Tests: object_watch.cpp (reentrancy, exceptions, mutation and collection inside a
callback), object_enumeration.c (shadowing, getters, flags, AVM1 and transport),
objects_transfer_process.c (independent file/TCP processes), standard_function.cpp,
standard_textformat.c, avm1_standard_types.cpp, serialization_validation.c (real
v1/v2 records), error_allocation.cpp (allocation failure boundaries).

# Наблюдение свойств и перечисление

Object.watch получает имя, функцию и userData. Обработчик вызывается с this,
равным наблюдаемому объекту, и аргументами (имя, старое значение, запрошенное
значение, userData). Возвращённое значение записывается в свойство. unwatch
возвращает факт удаления обработчика. Удаление свойства сохраняет наблюдение;
повторный вход в тот же обработчик подавляется. Исключение отменяет ожидающую
запись, но уже выполненные действия обработчика сохраняются. Read-only поля не
вызывают обработчик. Accessor-свойства и нативные поля трансформации MovieClip
не наблюдаются. Индексные записи Array участвуют; удаление элементов не вызывает
watch. Корни GC удерживаются даже при unwatch и сборке внутри обработчика.

Формат графа 3 переносит обработчики через очередь, файл и TCP. Читатель принимает
старые форматы 1/2. Получателю не требуется установка стандартных глобальных
объектов. Watches на ресурсных capability-объектах пока отклоняются с UNSUPPORTED,
чтобы не терять данные. Перенос таких аннотаций ещё предстоит реализовать.

[fvm_object_enumerate](../include/flashvm/property.h#L41) возвращает снимок перечислимых имён Array, по выбору с
прототипами. Собственное неперечислимое имя скрывает унаследованное, getters не
вызываются. Enumerate/Enumerate2 используют снимок и null-маркер в стеке.
Порядок определён реализацией и не заявляется идентичным старым Flash.

Function.call/apply сохраняют конструкторы упаковки примитивов при переносе.
TextFormat стал переносимым конструктором-функцией. TextField через new не
создаётся. Разрешение переменных учитывает undefined в локальной области и
свойства this. Отрицательные числовые ключи становятся именами, положительные
индексы старых нативных таблиц продолжают работать через индексный API.

Перечисленные выше тесты проверяют эти возможности и отказы выделения памяти.
Полная готовность Object/Array/Function/MovieClip этим этапом не заявляется.

## v0.478 — callback argument projection / преобразование аргумента callback

For loaded SWF 4–8, watch first converts its name to a string, then converts the
callback to an object through the active global Number/Boolean/String constructor.
An existing object is unchanged. Missing/nullish/unboxable callbacks get a fresh
bare object and fail the Function test. A native boxing constructor can return an
actual Function, which is installed as the handler with the original userData.
Nonfunction results return false without replacing an existing watcher; errors
and exceptions propagate before replacement. The unloaded host API still requires
an already callable object. No-argument watch still returns false directly.

The native watch barrier stays active during argument conversion. The projected
Function is explicitly rooted until its descriptor owns it. Removing the boxing
global and collecting afterwards preserves an installed handler; graph transfer
to a loaded context without standard installation also preserves its behavior.
Name conversion can change the selected global constructor. A failed name
conversion must prevent callback conversion entirely. Native error output keeps
the existing convention: undefined if entry was not reached, otherwise the
initial false may remain; callers must use the result status.

В SWF 4–8 watch сначала преобразует имя, затем callback через текущий глобальный
конструктор. Если фабрика вернула Function, она устанавливается с исходным
userData. Необъектный или невызываемый результат даёт false и сохраняет прежний
watcher; ошибки и исключения также не заменяют его. Без SWF принимается только
готовая вызываемая функция. Вызов без аргументов сразу возвращает false.
Преобразованная функция защищена корнем до записи дескриптора. Проверяются
порядок преобразований, удаление boxing-глобала и GC, перенос обработчика,
сохранение старого watcher при каждом отказе памяти и успешный повтор.

Tests: object_watch_global_boxing, object_watch_projected_function,
object_watch_boxing_order, object_watch_boxing_allocation.
Historical v0.14 call/apply boxing notes above describe host behavior; loaded
current-global behavior is documented in PRIMITIVE_GLOBAL_BOXING.md (v0.474).
Полная совместимость watch, version visibility, транспорта ресурсов и всех
этапов роадмапа этим исправлением не заявляется.

Loaded watcher depth is explicitly zero: [v0.502](WATCH_CALLBACK_SUPER.md).

Version-hidden old-value storage is skipped in [v0.503](WATCH_OLD_VALUE_VISIBILITY.md).

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_object_enumerate` | function | [include/flashvm/property.h:41](../include/flashvm/property.h#L41) |
<!-- source-links:end -->
