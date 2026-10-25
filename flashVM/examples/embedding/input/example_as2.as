/* FlashVM: Embedding/ActionScript tutorial: example as2.
* File: examples/embedding/input/example_as2.as.
*/

// ActionScript 2 (SWF 7/8): conceptual target of translated mouse events.
// The input mapper currently returns a button to the host; it does NOT yet dispatch it to AVM1.
/* Объявляем listener и сохраняем начальное значение. */
var listener: Object = new Object(); // Объявляем listener и сохраняем начальное значение.
/* Объявляем функцию или назначаем обработчик события. */
listener.onMouseDown = function() // Объявляем функцию или назначаем обработчик события.
/* Начинаем блок инструкций или описание объекта. */
{ // Begin the documented branch/scope: Начинаем блок инструкций или описание объекта.
    /* Вызываем trace: выводим диагностическое представление значения.
    * "mouse down" — значение или выражение для вывода. */
    trace("mouse down"); // Вызываем trace: выводим диагностическое представление значения. "mouse down" — значение или выражение для вывода.
    /* Завершаем текущий блок или описание объекта. */
}; // Завершаем текущий блок или описание объекта.
/* Вызываем Mouse.addListener: подключаем объект с обработчиками событий мыши.
* listener — объект слушателя. */
Mouse.addListener(listener); // Вызываем Mouse.addListener: подключаем объект с обработчиками событий мыши. listener — объект слушателя.
