/* FlashVM: Embedding/ActionScript tutorial: gesture.
 * File: examples/embedding/gesture_recognizer/gesture.as.
 */

// ActionScript 2.0 — planned integration, NOT executed by this example.
// ActionScript 2.0 — планируемая интеграция, пример пока не исполняется.
// After the future AVM1 mouse bridge is implemented, a two-finger tap
// mapped to the right mouse button can be delivered to an object handler.
// После реализации моста AVM1 событие сможет попасть в обработчик объекта.
/* Объявляем функцию или назначаем обработчик события. */
_root.onRightClick = function()
/* Начинаем блок инструкций или описание объекта. */
{
    /* Выполняем вызов trace с указанными аргументами. */
    trace("two-finger tap / тап двумя пальцами");
    /* Завершаем текущий блок или описание объекта. */
};
