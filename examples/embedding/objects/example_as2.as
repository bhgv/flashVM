/* FlashVM: Embedding/ActionScript tutorial: example as2.
 * File: examples/embedding/objects/example_as2.as.
 */

// ActionScript 2 (SWF 7/8): host has already exposed gameState.
/* Вызываем trace: выводим диагностическое представление значения.
 * gameState.score — значение или выражение для вывода. */
trace(gameState.score);
/* Сохраняем вычисленное значение: gameState.score. */
gameState.score = 456;
