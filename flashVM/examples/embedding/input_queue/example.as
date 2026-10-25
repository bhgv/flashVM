/* FlashVM: Embedding/ActionScript tutorial: example.
* File: examples/embedding/input_queue/example.as.
*/

// ActionScript 2.0 — DESIGN EXAMPLE, NOT YET EXECUTABLE BY FLASHVM.
// Future FlashVM touch events will be dispatched to MovieClip handlers.
/* Объявляем функцию или назначаем обработчик события. */
_root.onMultiTap = function(fingers, x, y) // Объявляем функцию или назначаем обработчик события.
/* Начинаем блок инструкций или описание объекта. */
{ // Begin the documented branch/scope: Начинаем блок инструкций или описание объекта.
    /* Проверяем условие перед выполнением следующей ветки. */
    if (fingers == 2) // Проверяем условие перед выполнением следующей ветки.
    /* Вызываем trace: выводим диагностическое представление значения.
    * "Two-finger tap at " + x + ", " + y — значение или выражение для вывода. */
    { // Begin the documented branch/scope: * "Two-finger tap at " + x + ", " + y — значение или выражение для вывода.
        trace("Two-finger tap at " + x + ", " + y); // Вызываем trace: выводим диагностическое представление значения. "Two-finger tap at " + x + ", " + y — значение или выражение для вывода.
    } // End the documented branch/scope.

    /* Завершаем текущий блок или описание объекта. */
}; // Завершаем текущий блок или описание объекта.
/* Объявляем функцию или назначаем обработчик события. */
_root.onSwipe = function(direction, fingers, x, y) // Объявляем функцию или назначаем обработчик события.
/* Начинаем блок инструкций или описание объекта. */
{ // Begin the documented branch/scope: Начинаем блок инструкций или описание объекта.
    /* Вызываем trace: выводим диагностическое представление значения.
    * "Swipe: " + direction — значение или выражение для вывода. */
    trace("Swipe: " + direction); // Вызываем trace: выводим диагностическое представление значения. "Swipe: " + direction — значение или выражение для вывода.
    /* Завершаем текущий блок или описание объекта. */
}; // Завершаем текущий блок или описание объекта.
