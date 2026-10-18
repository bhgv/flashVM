/* FlashVM: Embedding/ActionScript tutorial: example as2.
 * File: examples/embedding/drawing/example_as2.as.
 */

// AS2 (SWF 7/8): planned MovieClip drawing API.
/* Объявляем pen и сохраняем начальное значение. */
var pen : MovieClip = _root.createEmptyMovieClip("pen", 11);
/* Вызываем pen.lineStyle: задаём толщину, цвет и прозрачность границы.
 * 2 — толщина линии в пикселях.
 * 0x00FF00 — цвет RGB.
 * 100 — прозрачность в процентах. */
pen.lineStyle(2, 0x00FF00, 100);
/* Вызываем pen.moveTo: перемещаем текущую точку рисования.
 * 0 — координата x.
 * 0 — координата y. */
pen.moveTo(0, 0);
/* Вызываем pen.lineTo: добавляем прямой сегмент от текущей точки.
 * 100 — координата x конечной точки.
 * 100 — координата y конечной точки. */
pen.lineTo(100, 100);
