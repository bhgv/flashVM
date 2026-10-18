/* FlashVM: Embedding/ActionScript tutorial: example.
 * File: examples/embedding/input_queue/example.as.
 */

// ActionScript 2.0 — DESIGN EXAMPLE, NOT YET EXECUTABLE BY FLASHVM.
// Future FlashVM touch events will be dispatched to MovieClip handlers.
/* Объявляем функцию или назначаем обработчик события. */
_root.onMultiTap = function(fingers, x, y)
/* Начинаем блок инструкций или описание объекта. */
{
    /* Проверяем условие перед выполнением следующей ветки. */
    if (fingers == 2)
        /* Вызываем trace: выводим диагностическое представление значения.
 * "Two-finger tap at " + x + ", " + y — значение или выражение для вывода. */
        trace("Two-finger tap at " + x + ", " + y);
    /* Завершаем текущий блок или описание объекта. */
};
/* Объявляем функцию или назначаем обработчик события. */
_root.onSwipe = function(direction, fingers, x, y)
/* Начинаем блок инструкций или описание объекта. */
{
    /* Вызываем trace: выводим диагностическое представление значения.
 * "Swipe: " + direction — значение или выражение для вывода. */
    trace("Swipe: " + direction);
    /* Завершаем текущий блок или описание объекта. */
};
