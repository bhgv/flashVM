/* FlashVM: Embedding/ActionScript tutorial: example.
* File: examples/embedding/os_ports/example.as.
*/

// ActionScript 2.0 illustrative host integration; NOT wired to AVM1 yet.
// A host polls native input through ports/, then calls fvm_input_dispatch.
// TCP sockets currently have a C host API, not a built-in AS Socket class.
/* Объявляем функцию или назначаем обработчик события. */
_root.onMouseMove = function() // Объявляем функцию или назначаем обработчик события.
/* Начинаем блок инструкций или описание объекта. */
{ // Begin the documented branch/scope: Начинаем блок инструкций или описание объекта.
    /* Вызываем trace: выводим диагностическое представление значения.
    * "mouse moved" — значение или выражение для вывода. */
    trace("mouse moved"); // Вызываем trace: выводим диагностическое представление значения. "mouse moved" — значение или выражение для вывода.
    /* Завершаем текущий блок или описание объекта. */
}; // Завершаем текущий блок или описание объекта.
