/* FlashVM: Embedding/ActionScript tutorial: example.
* File: examples/embedding/prototypes/example.as.
*/

// ActionScript 2.0 (SWF 7/8): conceptual equivalent of the C example.
/* Initialize Base and its owned state. */


/* Объявляем функцию или назначаем обработчик события. */
function Base() // Объявляем функцию или назначаем обработчик события.
/* Начинаем блок инструкций или описание объекта. */
{ // Begin the documented branch/scope: Начинаем блок инструкций или описание объекта.
    /* Завершаем текущий блок или описание объекта. */
} // End the documented branch/scope.

/* Сохраняем вычисленное значение: Base.prototype.inherited. */
Base.prototype.inherited = 42; // Сохраняем вычисленное значение: Base.prototype.inherited.

/* Initialize Child and its owned state. */


/* Объявляем функцию или назначаем обработчик события. */
function Child() // Объявляем функцию или назначаем обработчик события.
/* Начинаем блок инструкций или описание объекта. */
{ // Begin the documented branch/scope: Начинаем блок инструкций или описание объекта.
    /* Завершаем текущий блок или описание объекта. */
} // End the documented branch/scope.

/* Вызываем Base: вызываем базовый конструктор. */
Child.prototype = new Base(); // Вызываем Base: вызываем базовый конструктор.
/* Объявляем instance и сохраняем начальное значение. */
var instance = new Child(); // Объявляем instance и сохраняем начальное значение.
/* Вызываем trace: выводим диагностическое представление значения.
* instance.inherited — значение или выражение для вывода. */
trace(instance.inherited); // Вызываем trace: выводим диагностическое представление значения. instance.inherited — значение или выражение для вывода.
// NOTE: DefineFunction/NewObject for arbitrary AS constructors is not yet supported.
