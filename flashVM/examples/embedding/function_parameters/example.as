/* FlashVM: Embedding/ActionScript tutorial: example.
* File: examples/embedding/function_parameters/example.as.
*/

// ActionScript 1/2: named functions with parameters.
// This subset is executable by the current FlashVM AVM1 interpreter.
/* Implement echo for this subsystem. */


/* Объявляем функцию или назначаем обработчик события. */
function echo(n) // Объявляем функцию или назначаем обработчик события.
/* Начинаем блок инструкций или описание объекта. */
{ // Begin the documented branch/scope: Начинаем блок инструкций или описание объекта.
    /* Возвращаем результат вызывающей стороне и завершаем функцию. */
    return n; // Возвращаем результат вызывающей стороне и завершаем функцию.
    /* Завершаем текущий блок или описание объекта. */
} // End the documented branch/scope.

/* Объявляем result и сохраняем начальное значение. */
var result = echo(42); // Объявляем result и сохраняем начальное значение.
