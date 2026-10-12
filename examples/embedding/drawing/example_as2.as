/* FlashVM: Embedding/ActionScript tutorial: example as2.
 * File: examples/embedding/drawing/example_as2.as.
 */

// AS2 (SWF 7/8): planned MovieClip drawing API.
/* Объявляем pen и сохраняем начальное значение. */
var pen : MovieClip = _root.createEmptyMovieClip("pen", 11);
/* Выполняем вызов pen.lineStyle с указанными аргументами. */
pen.lineStyle(2, 0x00FF00, 100);
/* Выполняем вызов pen.moveTo с указанными аргументами. */
pen.moveTo(0, 0);
/* Выполняем вызов pen.lineTo с указанными аргументами. */
pen.lineTo(100, 100);
