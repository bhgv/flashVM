/* FlashVM: Embedding/ActionScript tutorial: string.
 * File: examples/actionscript/string.as.
 */

// API illustration; executable coverage: tests/standard_string_avm1.cpp.
// Иллюстрация API; исполняемая проверка: tests/standard_string_avm1.cpp.
/* Объявляем text и сохраняем начальное значение. */
var text = new String("AБ😀Z");
/* Выполняем вызов trace с указанными аргументами. */
trace(text.length); // 5 UTF-16 units / кодовых единиц UTF-16
/* Выполняем вызов trace с указанными аргументами. */
trace(text.substring(2, 4)); // 😀
/* Выполняем вызов trace с указанными аргументами. */
trace(text.indexOf("Z")); // 4
/* Выполняем вызов trace с указанными аргументами. */
trace(text.charCodeAt(2)); // 55357 (high surrogate / старший суррогат)
/* Выполняем вызов trace с указанными аргументами. */
trace(String.fromCharCode(65)); // A
// text.charAt(2) is currently unsupported: isolated surrogate cannot be stored.
// text.charAt(2) пока не поддержан: одиночный суррогат не представим в C ABI.
