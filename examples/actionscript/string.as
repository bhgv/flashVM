/* FlashVM: Embedding/ActionScript tutorial: string.
 * File: examples/actionscript/string.as.
 */

// API illustration; executable coverage: tests/standard_string_avm1.cpp.
// Иллюстрация API; исполняемая проверка: tests/standard_string_avm1.cpp.
/* Объявляем text и сохраняем начальное значение. */
var text = new String("AБ😀Z");
/* Вызываем trace: выводим диагностическое представление значения.
 * text.length — значение или выражение для вывода. */
trace(text.length); // 5 UTF-16 units / кодовых единиц UTF-16
/* Вызываем trace: выводим диагностическое представление значения.
 * text.substring(2, 4) — значение или выражение для вывода. */
trace(text.substring(2, 4)); // 😀
/* Вызываем trace: выводим диагностическое представление значения.
 * text.indexOf("Z") — значение или выражение для вывода. */
trace(text.indexOf("Z")); // 4
/* Вызываем trace: выводим диагностическое представление значения.
 * text.charCodeAt(2) — значение или выражение для вывода. */
trace(text.charCodeAt(2)); // 55357 (high surrogate / старший суррогат)
/* Вызываем trace: выводим диагностическое представление значения.
 * String.fromCharCode(65) — значение или выражение для вывода. */
trace(String.fromCharCode(65)); // A
// text.charAt(2) is currently unsupported: isolated surrogate cannot be stored.
// text.charAt(2) пока не поддержан: одиночный суррогат не представим в C ABI.
