/* FlashVM: Embedding/ActionScript tutorial: parsing.
 * File: examples/actionscript/parsing.as.
 */

// API illustration; executable coverage: tests/global_parsing_avm1.cpp.
// Иллюстрация API; исполняемая проверка: tests/global_parsing_avm1.cpp.
/* Выполняем вызов trace с указанными аргументами. */
trace(parseInt("0777")); // 511, legacy AS2 octal / восьмеричное правило AS2
/* Выполняем вызов trace с указанными аргументами. */
trace(parseInt("0777", 10)); // 777
/* Выполняем вызов trace с указанными аргументами. */
trace(parseInt("3E8", 16)); // 1000
/* Выполняем вызов trace с указанными аргументами. */
trace(parseFloat("12.5e1px")); // 125
/* Выполняем вызов trace с указанными аргументами. */
trace(String(42)); // "42"
