# AVM1 function frames / Кадры вызова функций

## English

The interpreter now accepts named `ActionDefineFunction` (0x9B) parameter names.
Each invocation gets a separate local-variable map initialized with the supplied
arguments (missing arguments become `undefined`). `ActionGetVariable` searches
locals first, then globals. `ActionSetVariable` updates an existing local;
otherwise it writes a global. `ActionDefineLocal` (0x3C) and
`ActionDefineLocal2` (0x41) declare local bindings inside a function.
The existing maximum nesting depth is 64. This is **not** full AVM1 scope
semantics: closures, `with`, `this`, registers, `DefineFunction2`, method
calls, and proper SWF-version coercion are not yet implemented. Local
string values are not separately owned by the frame; VM lifetime rules apply.

## Русский

Интерпретатор теперь читает имена параметров `ActionDefineFunction` (0x9B).
Каждый вызов получает собственную таблицу локальных переменных, заполненную
аргументами (отсутствующие аргументы имеют значение `undefined`).
`ActionGetVariable` сначала ищет локальную переменную, затем глобальную.
`ActionSetVariable` обновляет существующую локальную переменную, иначе
записывает глобальную. `ActionDefineLocal` (0x3C) и `ActionDefineLocal2`
(0x41) объявляют локальные переменные внутри функции. Максимальная глубина
вызовов — 64. **Полная семантика AVM1 пока отсутствует:** замыкания,
`with`, `this`, регистры, `DefineFunction2`, вызовы методов и приведение
типов с учётом версии SWF ещё не реализованы.

Example / Пример: `examples/embedding/function_parameters/example.as`.
