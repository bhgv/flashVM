# C API расширений по образцу Lua

Образец: https://www.lua.org/manual/5.1/manual.html#3 . Использованы соглашения
Lua об индексах стека и callback-функциях; бинарной совместимости с Lua нет.
Заголовок: `flashvm/auxlib.h`. Пример: `examples/extending/lua_style/main.c`.

## Регистрация и результаты

`fvm_stack_register_cfunction(context, name, callback, capture)` регистрирует
callback типа `int (*)(FVM_Stack *)`. Необязательный объект capture принадлежит
контексту и остаётся корнем GC до замены регистрации. Захваченные VM-значения
нужно хранить в его свойствах: произвольная память C не сканируется.
Вызов доступен через [`fvm_call`](../include/flashvm/value.h#L138), [`fvm_stack_call`](../include/flashvm/stack.h#L50) и AVM1 ActionCallFunction.
Нативные [FVM_NativeFunction](../include/flashvm/value.h#L37) и скриптовые функции имеют приоритет при совпадении
имён. Запись регистрации копируется до вызова: callback может заменить себя.

При входе стек содержит аргументы. Их необязательно удалять: положите результат
на вершину и верните 1; возврат 0 означает undefined. Остальные элементы после
вызова удаляются. AVM1 возвращает одно значение, поэтому число результатов >1
отклоняется. Отрицательный [FVM_Result](../include/flashvm/flashvm.h#L40) сообщает ошибку. `longjmp` через C++-кадры
не применяется, исключения C++ перехватываются мостом. Ограничение повторного
входа в GC из обычных нативных callback сохраняется.

## Работа со стеком

| Функции | Поведение |
| --- | --- |
| size / settop | Число элементов / изменение размера, новые элементы undefined |
| pushvalue | Копирование элемента на вершину |
| remove / insert / replace | Удаление / перенос вершины в позицию / замена и удаление вершины |
| pushnumber / pushboolean / pushstring | Добавление значения; строка копируется |
| checknumber / checkstring | Строгая проверка типа, ошибка через [FVM_Result](../include/flashvm/flashvm.h#L40) |
| newtable | Создание объекта, добавление в трассируемый стек |
| getfield / setfield | Работа со свойством объекта; setfield удаляет вершину только при успехе |
| getglobal / setglobal | Работа с глобальными значениями |
| context | Получение контекста-владельца |

Все имена в таблице имеют префикс `fvm_stack_`. Индексы: 1 — первый элемент,
-1 — вершина, 0 недопустим. `settop(-1)` оставляет размер, `settop(-2)` удаляет
один элемент. Старый [FVM_StackFunction](../include/flashvm/stack.h#L16) API сохранён: возвращает [FVM_Result](../include/flashvm/flashvm.h#L40),
требует удалить аргументы и оставить не более одного элемента.

## Время жизни и ограничения

Стек уничтожается раньше контекста и не разделяется между потоками. Объекты
в стеке трассируются GC. `pushstring` копирует строку, низкоуровневый `push`
сохраняет прежний контракт заимствованной строки. Строки освобождаются при
уничтожении контекста; отдельной сборки строк пока нет. Возвращённый объект
нужно закреплять в трассируемых значениях или явных корнях перед следующим GC.

Ещё не реализованы: регистрация модулей, псевдоиндексы, C closure/upvalues,
бинарные строки, registry, несколько результатов для хоста, объекты ошибок
и универсальный вызов callable-свойств. Это первый этап Lua-подобного API.

`tests/auxlib.c` проверяет операции, отрицательные индексы и INT_MIN, ошибки
типов, копирование строк, сохранение объектов при GC, количество результатов
и вызов из настоящего AVM1-байткода. Новый пример включён в общий набор тестов.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_call` | function | [include/flashvm/value.h:138](../include/flashvm/value.h#L138) |
| `FVM_NativeFunction` | callback typedef | [include/flashvm/value.h:37](../include/flashvm/value.h#L37) |
| `FVM_Result` | enum | [include/flashvm/flashvm.h:40](../include/flashvm/flashvm.h#L40) |
| `FVM_Stack` | struct | [src/capi/stack_c.cpp:16](../src/capi/stack_c.cpp#L16) |
| `fvm_stack_call` | function | [include/flashvm/stack.h:50](../include/flashvm/stack.h#L50) |
| `fvm_stack_register_cfunction` | function | [include/flashvm/auxlib.h:18](../include/flashvm/auxlib.h#L18) |
| `FVM_StackFunction` | callback typedef | [include/flashvm/stack.h:16](../include/flashvm/stack.h#L16) |
<!-- source-links:end -->
