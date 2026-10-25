# AVM1 native methods / Нативные методы AVM1

## English

The public C89 ABI now exposes `[fvm_object_call](../include/flashvm/value.h#L184)(context, receiver, name,
argc, argv, result)`. It looks up `name` in the context's native function
registry and invokes the callback with `receiver` as `self`. The receiver must
belong to the same context; arguments are validated and the callback result
is returned to the caller. Errors are reported as [`FVM_Result`](../include/flashvm/flashvm.h#L42).

The interpreter implements AVM1 opcode `0x52` (`CallMethod`) for an object
receiver and a string method name. Its stack order is arguments, argument
count, receiver, method name (top). It returns the native callback's result.
Missing native registrations return [`FVM_UNSUPPORTED`](../include/flashvm/flashvm.h#L48). The interpreter also
supports resolving `this` to the current receiver for its limited global
function execution; full AVM1 lexical `this` semantics are not implemented.

**Not supported:** function-valued properties, `DefineFunction2`, complete
scope/closure semantics, display-object dispatch, and automatic gesture
callbacks. The callback registry is context-wide, not per-object; check
`self` in callbacks when appropriate. This API is not thread-safe.

## Русский

Публичный C89 ABI содержит `[fvm_object_call](../include/flashvm/value.h#L184)(context, receiver, name,
argc, argv, result)`. Функция ищет `name` в реестре нативных C-функций
контекста и вызывает обработчик, передавая `receiver` как `self`.
Объект должен принадлежать тому же контексту. Аргументы проверяются,
результат передаётся вызывающему коду, ошибки возвращаются через [`FVM_Result`](../include/flashvm/flashvm.h#L42).

Интерпретатор поддерживает инструкцию AVM1 `0x52` (`CallMethod`) для
объекта и строкового имени метода. Порядок стека: аргументы, их число,
объект, имя метода (вершина). Результат C-функции возвращается в стек.
Если нативная функция не зарегистрирована, возвращается [`FVM_UNSUPPORTED`](../include/flashvm/flashvm.h#L48).
Ограниченное разрешение `this` реализовано для глобальных функций;
полная семантика областей видимости AVM1 пока отсутствует.

**Не поддерживаются:** функции в свойствах объектов, `DefineFunction2`,
замыкания, методы MovieClip и автоматическая диспетчеризация жестов.
Реестр C-функций общий для контекста, а не индивидуальный для объекта.
При необходимости проверяйте `self` в обработчике. API не потокобезопасен.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_object_call` | function | [include/flashvm/value.h:184](../include/flashvm/value.h#L184) |
| `FVM_Result` | enum | [include/flashvm/flashvm.h:42](../include/flashvm/flashvm.h#L42) |
| `FVM_UNSUPPORTED` | enum constant | [include/flashvm/flashvm.h:48](../include/flashvm/flashvm.h#L48) |
<!-- source-links:end -->
