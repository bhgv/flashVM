/* FlashVM: Embedding/ActionScript tutorial: example.
* File: examples/embedding/script_methods/example.as.
*/

// ActionScript 1/2 illustration of the supported subset.
// The host creates _global.receiver and sets receiver.base = 35.
/* Implement compute for this subsystem. */


/* Объявляем функцию или назначаем обработчик события. */
function compute(n) // Объявляем функцию или назначаем обработчик события.
/* Начинаем блок инструкций или описание объекта. */
{ // Begin the documented branch/scope: Начинаем блок инструкций или описание объекта.
    /* Возвращаем результат вызывающей стороне и завершаем функцию. */
    return this.base + n; // Возвращаем результат вызывающей стороне и завершаем функцию.
    /* Завершаем текущий блок или описание объекта. */
} // End the documented branch/scope.

/* Объявляем result и сохраняем начальное значение. */
var result = receiver.compute(7); // 42
// Note: FlashVM currently resolves the named function from its global
// script-function registry; it does NOT yet store a function value on receiver.
