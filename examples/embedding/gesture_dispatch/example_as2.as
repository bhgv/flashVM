/* FlashVM: Embedding/ActionScript tutorial: example as2.
 * File: examples/embedding/gesture_dispatch/example_as2.as.
 */

// ActionScript 2.0: DESIGN TARGET, NOT YET SUPPORTED by the runtime.
// Native gesture callbacks currently reach C only; AVM1 event dispatch
// requires MovieClip hit-testing and the Flash event bridge.
/* Объявляем функцию или назначаем обработчик события. */
_root.onTap = function(fingers, x, y)
/* Начинаем блок инструкций или описание объекта. */
{
    /* Вызываем trace: выводим диагностическое представление значения.
 * "tap " + fingers — значение или выражение для вывода. */
    trace("tap " + fingers);
    /* Завершаем текущий блок или описание объекта. */
};
/* Объявляем функцию или назначаем обработчик события. */
_root.onMultiTap = function(fingers, x, y)
/* Начинаем блок инструкций или описание объекта. */
{
    /* Вызываем trace: выводим диагностическое представление значения.
 * "multitap " + fingers — значение или выражение для вывода. */
    trace("multitap " + fingers);
    /* Завершаем текущий блок или описание объекта. */
};
/* Объявляем функцию или назначаем обработчик события. */
_root.onSwipe = function(fingers, direction, x, y)
/* Начинаем блок инструкций или описание объекта. */
{
    /* Вызываем trace: выводим диагностическое представление значения.
 * "swipe " + direction — значение или выражение для вывода. */
    trace("swipe " + direction);
    /* Завершаем текущий блок или описание объекта. */
};
