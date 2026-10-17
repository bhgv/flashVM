# AVM1 script methods / Скриптовые методы AVM1

## English

`DefineFunction` registers a named function with a parameter list and bytecode
body. `CallFunction` executes it with the global receiver; `CallMethod` now
executes the same registered function with the object receiver supplied by the
bytecode. Each invocation has independent parameter/local bindings and a
64-level recursion limit. `GetVariable("this")` returns the current receiver
when no local variable shadows it. `Return` propagates the top stack value.

The host can create objects with [`fvm_object_create`](../include/flashvm/value.h#L62), set their properties with
[`fvm_object_set`](../include/flashvm/value.h#L74), and expose them to bytecode via [`fvm_set_global`](../include/flashvm/value.h#L87). See
`examples/embedding/script_methods/` and `tests/avm1_script_method.c`.

**Not implemented:** lookup of script-function values in object properties,
first-class functions, lexical closures, `DefineFunction2`, Flash-compatible
scope resolution, and automatic mouse/touch event dispatch into AVM1. A named
method may be invoked on any receiver because lookup is by name only. Do not
interpret this as complete ActionScript method semantics.

## Русский

`DefineFunction` регистрирует именованную функцию с параметрами и байткодом.
`CallFunction` вызывает её с глобальным объектом в качестве `this`, а
`CallMethod` — с переданным объектом-получателем. У каждого вызова собственные
локальные переменные и параметры; глубина вызовов ограничена 64 уровнями.
`GetVariable("this")` возвращает текущий объект, если имя не перекрыто локальной
переменной. `Return` возвращает верхнее значение стека.

Приложение создаёт объекты через [`fvm_object_create`](../include/flashvm/value.h#L62), задаёт свойства через
[`fvm_object_set`](../include/flashvm/value.h#L74) и публикует объект через [`fvm_set_global`](../include/flashvm/value.h#L87). Рабочий тест на
C89 находится в `tests/avm1_script_method.c`, пример AS — в
`examples/embedding/script_methods/`.

**Не реализовано:** хранение функций ActionScript в свойствах объектов,
функции как значения, замыкания, `DefineFunction2`, точная семантика областей
видимости и автоматическая доставка событий мыши/касаний в AVM1. Поиск
именованного метода пока производится по общему реестру функций, а не по
объекту. Это ограниченное промежуточное решение.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_object_create` | function | [include/flashvm/value.h:62](../include/flashvm/value.h#L62) |
| `fvm_object_set` | function | [include/flashvm/value.h:74](../include/flashvm/value.h#L74) |
| `fvm_set_global` | function | [include/flashvm/value.h:87](../include/flashvm/value.h#L87) |
<!-- source-links:end -->
