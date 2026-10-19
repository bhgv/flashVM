# Стековый API расширений в стиле Lua (C89 и C++98)

API **вдохновлён Lua**, но не совместим с Lua на уровне исходного кода или ABI. Обработчик получает `FVM_Context*` конкретного интерпретатора и отдельный `FVM_Stack*` с аргументами.

## Интерфейс C

Подключите `flashvm/stack.h`. Зарегистрируйте обработчик через `fvm_stack_register(context, name, callback, userdata)`, вызовите его через `fvm_stack_call(context, name, argc, argv, result)`.

При вызове стек содержит ровно `argc` аргументов. Положительные индексы начинаются с 1, отрицательные отсчитываются от вершины (`-1` — последний элемент). Индекс 0 запрещён. Для чтения используйте [`fvm_stack_get`](../include/flashvm/stack.h#L25), добавления — [`fvm_stack_push`](../include/flashvm/stack.h#L29), удаления — [`fvm_stack_pop`](../include/flashvm/stack.h#L31), очистки — [`fvm_stack_clear`](../include/flashvm/stack.h#L33). Успешный обработчик возвращает [`FVM_OK`](../include/flashvm/flashvm.h#L42) и оставляет **ровно одно** значение результата. Пустой стек означает `undefined`; более одного значения пока считается ошибкой.

Объекты должны принадлежать текущему контексту. Строки в стеке **заимствуются**, а не копируются: память строки должна оставаться действительной на протяжении вызова. Нельзя сохранять указатель на стек после завершения обработчика. Для доступа к состоянию ActionScript используются существующие [`fvm_object_get`](../include/flashvm/value.h#L72), [`fvm_object_set`](../include/flashvm/value.h#L74), [`fvm_get_global`](../include/flashvm/value.h#L85), [`fvm_set_global`](../include/flashvm/value.h#L87).

Рабочий пример: `examples/extending/stack_c/main.c`.

## Интерфейс C++98

`flashvm/stack.hpp` предоставляет RAII-обёртку `flashvm::Stack`, которая освобождает стек при выходе из области видимости. Реализация использует только C++98. Пример: `examples/embedding/cpp98_stack/main.cpp`.

## Несколько интерпретаторов и потоки

Каждый контекст имеет собственный реестр функций. Можно создавать несколько контекстов в одном runtime, но **одновременное обращение к одному контексту из разных потоков запрещено** без внешней синхронизации. Обмен данными между контекстами должен организовывать хост. Указатели объектов нельзя передавать между контекстами.

## Ограничения

Это дополнительный интерфейс вызова C-функций. Пока нет полноценных значений-функций, замыканий Lua, userdata, registry references, нескольких возвращаемых значений, корутин и автоматического вызова зарегистрированных стековых функций из ActionScript. Старый [`FVM_NativeFunction`](../include/flashvm/value.h#L37) сохранён. Полной совместимости с Lua нет.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_Context` | struct | [src/core/runtime.hpp:247](../src/core/runtime.hpp#L247) |
| `fvm_get_global` | function | [include/flashvm/value.h:85](../include/flashvm/value.h#L85) |
| `FVM_NativeFunction` | callback typedef | [include/flashvm/value.h:37](../include/flashvm/value.h#L37) |
| `fvm_object_get` | function | [include/flashvm/value.h:72](../include/flashvm/value.h#L72) |
| `fvm_object_set` | function | [include/flashvm/value.h:74](../include/flashvm/value.h#L74) |
| `FVM_OK` | enum constant | [include/flashvm/flashvm.h:42](../include/flashvm/flashvm.h#L42) |
| `fvm_set_global` | function | [include/flashvm/value.h:87](../include/flashvm/value.h#L87) |
| `FVM_Stack` | struct | [src/capi/stack_c.cpp:16](../src/capi/stack_c.cpp#L16) |
| `fvm_stack_call` | function | [include/flashvm/stack.h:50](../include/flashvm/stack.h#L50) |
| `fvm_stack_clear` | function | [include/flashvm/stack.h:33](../include/flashvm/stack.h#L33) |
| `fvm_stack_get` | function | [include/flashvm/stack.h:25](../include/flashvm/stack.h#L25) |
| `fvm_stack_pop` | function | [include/flashvm/stack.h:31](../include/flashvm/stack.h#L31) |
| `fvm_stack_push` | function | [include/flashvm/stack.h:29](../include/flashvm/stack.h#L29) |
| `fvm_stack_register` | function | [include/flashvm/stack.h:35](../include/flashvm/stack.h#L35) |
<!-- source-links:end -->
