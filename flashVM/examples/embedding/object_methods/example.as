/* FlashVM: Embedding/ActionScript tutorial: example.
* File: examples/embedding/object_methods/example.as.
*/

// ActionScript 1/2, conceptual example of the intended CallMethod bridge.
// The host creates receiver and registers the native function "add".
// This call syntax is not yet a verified SWF integration scenario:
/* Объявляем answer и сохраняем начальное значение. */
var answer = receiver.add(7); // Объявляем answer и сохраняем начальное значение.
/* Вызываем trace: выводим диагностическое представление значения.
* answer — значение или выражение для вывода. */
trace(answer); // Expected: 42 when method binding is implemented.
