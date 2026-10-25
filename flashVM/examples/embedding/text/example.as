/* FlashVM: Embedding/ActionScript tutorial: example.
* File: examples/embedding/text/example.as.
*/

// ActionScript 2.0 conceptual comparison (not a FlashVM integration test).
// ActionScript strings use UTF-16-like character semantics; the native
// UTF-8 helper functions are NOT yet exposed to ActionScript automatically.
/* Объявляем greeting и сохраняем начальное значение. */
var greeting: String = "Hello, мир!"; // Объявляем greeting и сохраняем начальное значение.
/* Вызываем trace: выводим диагностическое представление значения.
* greeting.length — значение или выражение для вывода. */
trace(greeting.length); // Вызываем trace: выводим диагностическое представление значения. greeting.length — значение или выражение для вывода.
/* Вызываем trace: выводим диагностическое представление значения.
* greeting.indexOf("Hello") — значение или выражение для вывода. */
trace(greeting.indexOf("Hello")); // Вызываем trace: выводим диагностическое представление значения. greeting.indexOf("Hello") — значение или выражение для вывода.
