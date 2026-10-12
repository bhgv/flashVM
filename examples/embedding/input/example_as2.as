/* FlashVM: Embedding/ActionScript tutorial: example as2.
 * File: examples/embedding/input/example_as2.as.
 */

// ActionScript 2 (SWF 7/8): conceptual target of translated mouse events.
// The input mapper currently returns a button to the host; it does NOT yet dispatch it to AVM1.
/* Объявляем listener и сохраняем начальное значение. */
var listener : Object = new Object();
/* Объявляем функцию или назначаем обработчик события. */
listener.onMouseDown = function()
/* Начинаем блок инструкций или описание объекта. */
{
    /* Выполняем вызов trace с указанными аргументами. */
    trace("mouse down");
    /* Завершаем текущий блок или описание объекта. */
};
/* Выполняем вызов Mouse.addListener с указанными аргументами. */
Mouse.addListener(listener);
