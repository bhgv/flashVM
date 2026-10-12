/* FlashVM: Embedding/ActionScript tutorial: example.
 * File: examples/embedding/prototypes/example.as.
 */

// ActionScript 2.0 (SWF 7/8): conceptual equivalent of the C example.
/* Initialize Base and its owned state. */
/* Объявляем функцию или назначаем обработчик события. */
function Base()
/* Начинаем блок инструкций или описание объекта. */
{
    /* Завершаем текущий блок или описание объекта. */
}
/* Сохраняем вычисленное значение: Base.prototype.inherited. */
Base.prototype.inherited = 42;

/* Initialize Child and its owned state. */
/* Объявляем функцию или назначаем обработчик события. */
function Child()
/* Начинаем блок инструкций или описание объекта. */
{
    /* Завершаем текущий блок или описание объекта. */
}
/* Выполняем вызов Base с указанными аргументами. */
Child.prototype = new Base();
/* Объявляем instance и сохраняем начальное значение. */
var instance = new Child();
/* Выполняем вызов trace с указанными аргументами. */
trace(instance.inherited);
// NOTE: DefineFunction/NewObject for arbitrary AS constructors is not yet supported.
