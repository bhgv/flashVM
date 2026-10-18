/* FlashVM: учебный AS2 пример Point, Matrix и Rectangle. */
/* Подключаем имя Point из пространства flash.geom для создания координат. */
import flash.geom.Point;
/* Подключаем Matrix для аффинных преобразований координат. */
import flash.geom.Matrix;
/* Подключаем Rectangle для работы с областями и границами. */
import flash.geom.Rectangle;
/* Создаём точку с координатами x=3 и y=4. */
var source:Point = new Point(3, 4);
/* Создаём единичную матрицу без изменения исходной точки. */
var transform:Matrix = new Matrix();
/* Сдвигаем координаты матрицы на 10 вправо и на 20 вниз. */
transform.translate(10, 20);
/* Получаем новую точку (13, 24); source остаётся (3, 4). */
var moved:Point = transform.transformPoint(source);
/* Создаём прямоугольник с началом (10, 20), шириной 30 и высотой 40. */
var region:Rectangle = new Rectangle(10, 20, 30, 40);
/* Проверяем точку: верхняя и левая границы включаются, правая и нижняя исключаются. */
trace(region.containsPoint(moved));
/* Получаем новую точку правого нижнего угла (40, 60). */
trace(region.bottomRight);
/* Увеличиваем ширину на 4, высоту на 6, сохраняя центр прямоугольника. */
region.inflate(2, 3);
/* Печатаем новые начало и размер: (x=8, y=17, w=34, h=46). */
trace(region.toString());
/* Подключаем ColorTransform для изменения числовых цветовых каналов. */
import flash.geom.ColorTransform;
/* Создаём преобразование с единичными множителями и нулевыми смещениями. */
var tint:ColorTransform = new ColorTransform();
/* Задаём постоянный RGB-цвет: множители RGB становятся нулевыми, alpha сохраняется. */
tint.rgb = 0x336699;
/* Печатаем числовое упакованное значение RGB — 3368601. */
trace(tint.rgb);
/* Копируем все восемь каналов через документированную сигнатуру AS2-конструктора. */
var tintCopy:ColorTransform = new ColorTransform(tint.redMultiplier, tint.greenMultiplier, tint.blueMultiplier, tint.alphaMultiplier, tint.redOffset, tint.greenOffset, tint.blueOffset, tint.alphaOffset);
/* Печатаем все множители и смещения независимой копии. */
trace(tintCopy.toString());
/* Подключаем Transform для доступа к матрице и цвету существующего MovieClip. */
import flash.geom.Transform;
/* Создаём пустой клип на свободной глубине текущего экземпляра. */
var sample:MovieClip = this.createEmptyMovieClip("geometrySample", this.getNextHighestDepth());
/* Получаем обёртку, связанную с живым клипом; она не копирует сам клип. */
var live:Transform = sample.transform;
/* Берём независимый снимок локальной матрицы клипа. */
var pose:Matrix = live.matrix;
/* Меняем только снимок: сам клип пока остаётся на прежнем месте. */
pose.translate(10, 20);
/* Присваиваем изменённый снимок обратно и переносим клип. */
live.matrix = pose;
/* Применяем цветовые параметры; дальнейшие изменения tintCopy не изменят клип. */
live.colorTransform = tintCopy;
/* Печатаем матрицу с учётом всех родителей клипа. */
trace(live.concatenatedMatrix);
