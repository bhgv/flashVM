# Vector gradients / Векторные градиенты

`main.c` is a C89 tutorial with an explanation of every code line. It creates
an editable rectangle and fills it from opaque red to opaque blue. The program
prints the center's straight RGBA values; it does not create an image file.

Build the ordinary project with GRAPHICS_AGG or GRAPHICS_CAIRO enabled in DEFS.txt
and run `flashvm_examples_embedding_gradients_main`. NO_GRAPHICS is supported
as a demonstration mode and prints how to enable a raster backend.

Call `fvm_gradient_init` before setting the borrowed stop array. Stop ratios run
from 0 to 1 in nondecreasing order. The gradient matrix maps the normalized square
(-1,-1)..(1,1) to pixels, independently of the path. For a horizontal ramp covering
a surface of width W, set xx=W/2 and x0=W/2; set a nonzero yy so the transform stays
invertible. RADIAL uses distance from the center to the unit circle. Outside the
ramp, endpoint colors are retained (pad). RGBA multipliers/offsets apply after
interpolation. Stop memory must remain valid throughout the draw call.

The caller owns the path and pixel buffer. Destroy the path after use; the
gradient descriptor owns no resources and retains no pointers after drawing.
No interpreter context or GC roots are needed for this native vector API.
The common antialiased contour mask preserves holes with even_odd=1.

Русский: `main.c` — пример C89 с пояснением каждой строки. Он создаёт редактируемый
прямоугольник и рисует градиент от красного к синему, затем печатает RGBA центра.
Включите GRAPHICS_AGG или GRAPHICS_CAIRO в DEFS.txt, соберите проект и запустите
`flashvm_examples_embedding_gradients_main`. При NO_GRAPHICS пример объясняет,
как включить отрисовку.

Матрица переводит нормализованный квадрат -1..1 в пиксели. Для горизонтальной
рампы ширины W задайте xx=W/2 и x0=W/2; yy должен быть ненулевым для обратимости.
RADIAL измеряет расстояние от центра до единичной окружности. Позиции цветов
лежат в 0..1 и идут по возрастанию; одинаковые позиции создают резкий переход.
Вне рампы сохраняется крайний цвет. Цветовое преобразование применяется после
интерполяции. Массив цветов должен существовать до завершения отрисовки.

Путь и пиксели принадлежат программе; путь нужно освободить. Контекст VM и корни
GC этому нативному API не требуются. Ограничения и проверки описаны в
[SWF_GRADIENTS.md](../../../doc/SWF_GRADIENTS.md).
