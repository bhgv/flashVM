/* Учебный пример AS2: рисуем градиент через MovieClip.beginGradientFill с legacy box-матрицей. */
/* [GC:] Созданный клип удерживается родительским display list и переменной gradient_mc. */
var gradient_mc:MovieClip = this.createEmptyMovieClip("gradient_mc", 1); // Создаём дочерний клип на глубине 1.
/* [GC:] Массив цветов остаётся достижимым через переменную до копирования параметров заливки. */
var colors:Array = [0xFF0000, 0x0000FF]; // Задаём красный цвет в начале и синий в конце рампы.
/* [GC:] Массив непрозрачности передаётся как обычный управляемый объект ActionScript. */
var alphas:Array = [100, 100]; // Делаем обе контрольные точки полностью непрозрачными.
/* [GC:] Массив позиций удерживается переменной и не сохраняется заливкой по ссылке. */
var ratios:Array = [0, 255]; // Размещаем цвета на противоположных концах градиента.
/* [GC:] Объект матрицы принадлежит VM; beginGradientFill скопирует его числовые поля. */
var matrix:Object = {matrixType:"box", x:0, y:0, w:100, h:100, r:0}; // Задаём область 100x100 без поворота.
gradient_mc.beginGradientFill("linear", colors, alphas, ratios, matrix, "pad", "RGB", 0); // Копируем рампу, обычную RGB-интерполяцию и матрицу; focal здесь не используется.
colors[0] = 0x00FF00; // Изменяем исходный массив: уже выбранная заливка продолжает начинаться красным.
matrix.w = 200; // Изменяем исходную ширину: скопированная матрица заливки сохраняет ширину 100.
gradient_mc.moveTo(0, 0); // Начинаем контур в левом верхнем углу градиентной области.
gradient_mc.lineTo(100, 0); // Рисуем верхнюю сторону прямоугольника.
gradient_mc.lineTo(100, 100); // Рисуем правую сторону прямоугольника.
gradient_mc.lineTo(0, 100); // Рисуем нижнюю сторону прямоугольника.
gradient_mc.lineTo(0, 0); // Возвращаемся к началу и замыкаем левую сторону.
gradient_mc.endFill(); // Заканчиваем заливку; готовый контур сохраняет собственную рампу.
trace("gradient ready: " + gradient_mc); // Выводим имя клипа для проверки созданного экземпляра.

/* SWF8: независимая градиентная обводка наследует толщину из lineStyle. */
var stroke_mc:MovieClip = this.createEmptyMovieClip("stroke_mc", 2); // Родитель удерживает второй клип на отдельной глубине.
var stroke_colors:Array = [0xFF0000, 0x0000FF]; // Передаём два цвета, которые будут скопированы в стиль обводки.
var stroke_matrix:Object = {matrixType:"box", x:0, y:110, w:40, h:40, r:0}; // Задаём период меньше длины линии для видимого отражения.
stroke_mc.lineStyle(4); // Сначала включаем обводку толщиной четыре пикселя.
stroke_mc.lineGradientStyle("linear", stroke_colors, alphas, ratios, stroke_matrix, "reflect", "RGB", 0); // Копируем обычную RGB-рампу с отражённым продолжением без focal-смещения.
stroke_colors[0] = 0x00FF00; // Изменение массива не меняет выбранный красный цвет обводки.
stroke_mc.moveTo(0, 130); // Начинаем линию посередине вертикальной области матрицы.
stroke_mc.lineTo(100, 130); // Рисуем несколько отражённых периодов на одной линии.
stroke_mc.endFill(); // Завершаем только заливку; текущая градиентная обводка остаётся выбранной.

/* SWF8: сравниваем более светлую середину linearRGB с прежней RGB-рампой. */
var linear_mc:MovieClip = this.createEmptyMovieClip("linear_mc", 3); // Родитель удерживает отдельный клип для новой цветовой политики.
var linear_colors:Array = [0x000000, 0xFFFFFF]; // Задаём чёрный и белый без изменения общей alpha-рампы.
var linear_matrix:Object = {matrixType:"box", x:0, y:160, w:100, h:40, r:0}; // Размещаем копируемую матрицу ниже прежних примеров.
linear_mc.beginGradientFill("linear", linear_colors, alphas, ratios, linear_matrix, "pad", "linearRGB", 0); // Выбираем интерполяцию RGB в линейном свете и нулевой focal.
linear_mc.moveTo(0, 160); // Начинаем прямоугольник в верхнем левом углу матрицы.
linear_mc.lineTo(100, 160); // Добавляем верхнюю сторону.
linear_mc.lineTo(100, 200); // Добавляем правую сторону.
linear_mc.lineTo(0, 200); // Добавляем нижнюю сторону.
linear_mc.lineTo(0, 160); // Замыкаем контур явной линией.
linear_mc.endFill(); // Готовый path удерживает скопированный interpolation selector.

/* SWF8: выбираем противоположные внутренние focal для радиальной заливки и обводки. */
var focal_mc:MovieClip = this.createEmptyMovieClip("focal_mc", 4); // Родитель удерживает новый клип на отдельной глубине.
var focal_colors:Array = [0xFF0000, 0x0000FF]; // Передаём независимые красный и синий stops.
var focal_matrix:Object = {matrixType:"box", x:0, y:220, w:100, h:60, r:0}; // Задаём копируемую область радиального градиента.
focal_mc.beginGradientFill("radial", focal_colors, alphas, ratios, focal_matrix, "pad", "linearRGB", 0.5); // Смещаем focal заливки вправо внутри нормализованного круга.
focal_mc.lineStyle(4); // Сначала включаем обводку с толщиной четыре пикселя.
focal_mc.lineGradientStyle("radial", focal_colors, alphas, ratios, focal_matrix, "reflect", "RGB", -0.5); // Выбираем для обводки отдельный focal слева и отражённое продолжение.
focal_mc.moveTo(0, 220); // Начинаем контур до добавления его сторон.
focal_mc.lineTo(100, 220); // Рисуем верхнюю сторону с двумя сохранёнными рампами.
focal_mc.lineTo(100, 280); // Добавляем правую сторону.
focal_mc.lineTo(0, 280); // Добавляем нижнюю сторону.
focal_mc.lineTo(0, 220); // Возвращаемся в начало контура.
focal_mc.endFill(); // Готовый path сохраняет обе независимые focal-записи.
