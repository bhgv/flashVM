/* FlashVM: Embedding/ActionScript tutorial: parsing.
* File: examples/actionscript/parsing.as.
*/

// API illustration; executable coverage: tests/global_parsing_avm1.cpp.
// Иллюстрация API; исполняемая проверка: tests/global_parsing_avm1.cpp.
/* Вызываем trace: выводим диагностическое представление значения.
* parseInt("0777") — значение или выражение для вывода. */
trace(parseInt("0777")); // 511, legacy AS2 octal / восьмеричное правило AS2
/* Вызываем trace: выводим диагностическое представление значения.
* parseInt("0777", 10) — значение или выражение для вывода. */
trace(parseInt("0777", 10)); // 777
/* Вызываем trace: выводим диагностическое представление значения.
* parseInt("3E8", 16) — значение или выражение для вывода. */
trace(parseInt("3E8", 16)); // 1000
/* Вызываем trace: выводим диагностическое представление значения.
* parseFloat("12.5e1px") — значение или выражение для вывода. */
trace(parseFloat("12.5e1px")); // 125
/* Вызываем trace: выводим диагностическое представление значения.
* String(42) — значение или выражение для вывода. */
trace(String(42)); // "42"
