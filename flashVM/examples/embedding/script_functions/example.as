/* FlashVM: Embedding/ActionScript tutorial: example.
 * File: examples/embedding/script_functions/example.as.
 */

// ActionScript 1 / ActionScript 2 source illustration.
// A host can read the resulting global variable through fvm_get_global().
/* Implement answer for this subsystem. */
/* Объявляем функцию или назначаем обработчик события. */
function answer()
/* Начинаем блок инструкций или описание объекта. */
{
    /* Возвращаем результат вызывающей стороне и завершаем функцию. */
    return 42;
    /* Завершаем текущий блок или описание объекта. */
}
/* Объявляем x и сохраняем начальное значение. */
var x = answer();
