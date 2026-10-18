# Вызываемые объекты — C API

Заголовок: `flashvm/callable.h` (C89). Реализация: `src/capi/stack_c.cpp` (C++98).

- `fvm_callable_create(ctx, callback, userdata, &object)` создаёт обычный объект
  контекста с привязанной C-функцией. Сигнатура обработчика [`FVM_StackFunction`](../include/flashvm/stack.h#L16):
  `(context, stack, userdata)`.
- `fvm_callable_is(ctx, object, &flag)` проверяет наличие обработчика.
- `fvm_callable_invoke(ctx, object, argc, argv, &result)` создаёт временный стек
  аргументов и вызывает обработчик. Ноль результатов означает `undefined`, один
  результат допустим, больше одного — ошибка.

Объект можно записать в свойство через [`FVM_Value`](../include/flashvm/value.h#L24) с типом [`FVM_VALUE_OBJECT`](../include/flashvm/value.h#L21).
Нового типа значения пока нет. Замыкания ActionScript, метаметод `__call` и
вызов функций из свойств непосредственно байткодом AVM1 ещё не реализованы.

Владение: объекты и регистрации принадлежат контексту. Время жизни `userdata`
обеспечивает приложение. Передавать объекты между контекстами нельзя. Один
контекст нельзя использовать одновременно из нескольких потоков. Исключения
C++ из обработчиков преобразуются в [`FVM_ERROR`](../include/flashvm/flashvm.h#L43).

Пример на C89: `examples/extending/callable_objects/`.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_callable_create` | function | [include/flashvm/callable.h:19](../include/flashvm/callable.h#L19) |
| `fvm_callable_invoke` | function | [include/flashvm/callable.h:27](../include/flashvm/callable.h#L27) |
| `fvm_callable_is` | function | [include/flashvm/callable.h:25](../include/flashvm/callable.h#L25) |
| `FVM_ERROR` | enum constant | [include/flashvm/flashvm.h:43](../include/flashvm/flashvm.h#L43) |
| `FVM_StackFunction` | callback typedef | [include/flashvm/stack.h:16](../include/flashvm/stack.h#L16) |
| `FVM_Value` | struct | [include/flashvm/value.h:24](../include/flashvm/value.h#L24) |
| `FVM_VALUE_OBJECT` | enum constant | [include/flashvm/value.h:21](../include/flashvm/value.h#L21) |
<!-- source-links:end -->
