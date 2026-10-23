# Vector gradients / Векторные градиенты

`main.c` is a C89 tutorial with an explanation of every code line. It creates
an editable rectangle and fills it from opaque red to opaque blue. The program
prints the center's straight RGBA values; it does not create an image file.

Build the ordinary project with GRAPHICS_AGG or GRAPHICS_CAIRO enabled in DEFS.txt
and run `flashvm_examples_embedding_gradients_main`. NO_GRAPHICS is supported
as a demonstration mode and prints how to enable a raster backend.

Call [`fvm_gradient_init`](../../../include/flashvm/gradient.h#L47) before setting the borrowed stop array. Stop ratios run
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

The annotated AS2 example_as2.as demonstrates MovieClip.beginGradientFill with a copied legacy box matrix; see [RU/EN checkpoint](../../../doc/MOVIECLIP_GRADIENT_FILL.md).
Русский: example_as2.as поясняет вызов beginGradientFill, копирование аргументов и сохранение рампы готовым контуром.

## Gradient strokes / Градиентные обводки

The C89 `stroke.inc` companion reuses the owned rectangle, borrowed stops and RGBA buffer. It draws a four-pixel stroke with PAD, REFLECT and REPEAT, then prints red samples at phases 0.5, 1 and 2. No VM, managed root, device or file is retained. API errors clean up the path and return a failing process status; NO_GRAPHICS prints the existing backend message before this helper runs.

C89 helper сравнивает обводки с тремя режимами продолжения. Все массивы и путь принадлежат main; helper ничего не сохраняет. Ожидаемый дополнительный вывод:

```text
stroke pad: 128 0 0
stroke reflect: 128 0 255
stroke repeat: 128 255 255
```

| Function | Purpose and ownership / Назначение и владение |
| --- | --- |
| [`fvm_path_stroke_gradient`](../../../include/flashvm/gradient.h#L63) | Synchronously borrows path, gradient/stops and destination; paints a positive destination-space stroke. Invalid input or preparation failure preserves pixels. [Contract / Контракт](../../../doc/GRADIENT_STROKES.md). |

[Stroke contract / Контракт обводок](../../../doc/GRADIENT_STROKES.ru.md). This example checks the selected backend on the host; reference-player and physical-device acceptance remain separate.

Run `python3 flashVM/scripts/check_gradient_strokes_example.py <path-to-example>` from the repository root to check the actual printed samples. CTest registers `flashvm_gradient_strokes_tutorial`; NO_GRAPHICS returns explicit skip code 77. The accompanying AS2 source adds a separate SWF8 stroke clip; bytecode execution is tested by the AVM1 fixtures, not by compiling this illustrative source in the CMake build.

Проверка C89 вывода выполняется отдельным checker и CTest; NO_GRAPHICS явно пропускается. AS2 source показывает lineStyle перед lineGradientStyle и копирование массива цветов; его компиляция в SWF не входит в эту CMake-проверку.

## RGB and linearRGB / Цветовые политики

The annotated C89 `linear_rgb.inc` companion owns two local black/white stops and borrows main's path/buffer. It compares the same midpoint in fill and stroke coverage, from transparent black to opaque white. linearRGB changes the three color channels while alpha stays 128. Calls are synchronous and retain no VM, device or input pointers; errors return through main's normal path cleanup.

C89 helper сравнивает реальный midpoint, не меняя ABI layout [`FVM_Gradient`](../../../include/flashvm/gradient.h#L37). Straight alpha интерполируется линейно в обоих режимах. Дополнительный вывод:

```text
linear fill RGB: 128 128 128 128
linear fill linearRGB: 188 188 188 128
linear stroke RGB: 128 128 128 128
linear stroke linearRGB: 188 188 188 128
```

| Function | Purpose and ownership / Назначение и владение |
| --- | --- |
| [`fvm_path_draw_gradient_interpolated`](../../../include/flashvm/gradient.h#L70) | Borrow path, surface and gradient/stops until return; fill with unsigned spread/interpolation selectors. Invalid input or preparation failure preserves pixels. [Contract](../../../doc/LINEAR_RGB_GRADIENTS.md). |
| [`fvm_path_stroke_gradient_interpolated`](../../../include/flashvm/gradient.h#L76) | Borrow the same inputs for a positive destination-pixel stroke with explicit color policy; no VM/device/root is retained. [Контракт](../../../doc/LINEAR_RGB_GRADIENTS.ru.md). |

NO_GRAPHICS follows the earlier backend message and does not claim to render these values. Original-player and physical-platform comparisons remain separate.

Run `python3 flashVM/scripts/check_gradient_linear_rgb_example.py <path-to-example>` from the repository root. CTest registers `flashvm_gradient_linear_rgb_tutorial`; the backend-free executable explicitly skips with code 77. The checker requires all eight actual output lines, including the legacy fill and spread samples.

AS2 companion также показывает отдельный SWF8 linearRGB-клип. CMake не компилирует этот illustrative source в SWF; реальные AVM1 fixtures проверяют соответствующие операции и JIT отдельно. Checker проверяет все восемь строк C89-вывода, а без backend явно пропускается.

<!-- example-catalog-api:start -->
## Назначение и особенности

Создаёт редактируемый прямоугольный путь и линейную красно-синюю заливку, затем печатает центральный RGBA. Показывает матрицу градиента, диапазон stop ratios и владение массивом остановок; графический backend выбирается в DEFS.

**Текущая CMake-цель:** `flashvm_examples_embedding_gradients_main`.


## Демонстрируемые функции C API

Имена ведут к публичным декларациям. Во второй колонке указан файл руководства
и раздел с контрактом; общие функции setup/cleanup включены для воспроизводимости.

| Функция | Где читать подробнее |
| --- | --- |
| [`fvm_gradient_init`](../../../include/flashvm/gradient.h#L47) | [SWF_GRADIENTS.md](../../../doc/SWF_GRADIENTS.md) — Типы градиентов и координатные преобразования. |
| [`fvm_path_add`](../../../include/flashvm/path.h#L54) | [FONT_LIBRARIES.md#owned-vector-model](../../../doc/FONT_LIBRARIES.md#owned-vector-model) — Владеющие векторные пути и преобразования. |
| [`fvm_path_create`](../../../include/flashvm/path.h#L50) | [FONT_LIBRARIES.md#owned-vector-model](../../../doc/FONT_LIBRARIES.md#owned-vector-model) — Владеющие векторные пути и преобразования. |
| [`fvm_path_destroy`](../../../include/flashvm/path.h#L52) | [FONT_LIBRARIES.md#owned-vector-model](../../../doc/FONT_LIBRARIES.md#owned-vector-model) — Владеющие векторные пути и преобразования. |
| [`fvm_path_draw_gradient`](../../../include/flashvm/gradient.h#L51) | [SWF_GRADIENTS.md](../../../doc/SWF_GRADIENTS.md) — Типы градиентов и координатные преобразования. |

[Описание в общем каталоге](../../README.md#example-embedding-gradients).
<!-- example-catalog-api:end -->

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_Gradient` | struct | [include/flashvm/gradient.h:37](../../../include/flashvm/gradient.h#L37) |
| `fvm_gradient_init` | function | [include/flashvm/gradient.h:47](../../../include/flashvm/gradient.h#L47) |
| `fvm_path_add` | function | [include/flashvm/path.h:54](../../../include/flashvm/path.h#L54) |
| `fvm_path_create` | function | [include/flashvm/path.h:50](../../../include/flashvm/path.h#L50) |
| `fvm_path_destroy` | function | [include/flashvm/path.h:52](../../../include/flashvm/path.h#L52) |
| `fvm_path_draw_gradient` | function | [include/flashvm/gradient.h:51](../../../include/flashvm/gradient.h#L51) |
| `fvm_path_draw_gradient_interpolated` | function | [include/flashvm/gradient.h:70](../../../include/flashvm/gradient.h#L70) |
| `fvm_path_stroke_gradient` | function | [include/flashvm/gradient.h:63](../../../include/flashvm/gradient.h#L63) |
| `fvm_path_stroke_gradient_interpolated` | function | [include/flashvm/gradient.h:76](../../../include/flashvm/gradient.h#L76) |
<!-- source-links:end -->
