# Linear and radial gradients / Линейные и радиальные градиенты

Development checkpoint v0.30; roadmap stage 4 remains open.

DefineShape/2/3 support linear (0x10) and centered radial (0x12) gradient fills.
Their MATRIX and ordered RGB/RGBA control points are decoded with strict record
boundaries. As specified for these three tags, reserved spread/interpolation bits
must be zero and there must be 1..8 stops. Invalid counts, unsorted ratios, missing
bytes and trailing data are rejected. Focal gradients and bitmap fills remain
unsupported; Shape4 spread/interpolation modes are not implied by this checkpoint.

The managed paint stores copied stops and its matrix. No backend handles or
borrowed file pointers are retained. Rendering composes the gradient's independent
matrix with the complete scene transform, samples straight RGBA at pixel centers,
then applies inherited color multipliers/offsets to the interpolated result.
An antialiased vector mask clips the paint and preserves even-odd holes. AGG and
Cairo share the same ramp evaluation. Colors outside the ramp use its endpoint
values (pad). Duplicate stop positions select the last stop at that exact ratio.

The C89 API in `include/flashvm/gradient.h` exposes the same linear/radial pad
painting for ordinary editable paths. Initialize [FVM_Gradient](../include/flashvm/gradient.h#L30), supply a borrowed
ordered array of 1..15 stops and its matrix, and call [fvm_path_draw_gradient](../include/flashvm/gradient.h#L44).
The matrix maps the normalized square -1..1 to destination pixels; SWF's square
of +/-16384 twips is normalized during scene rendering. Matrices are bounded to
absolute components <=1,000,000 and require absolute determinant >=1e-20.
Post-interpolation RGBA multipliers/offsets are bounded to +/-65536. The existing
raster coordinate/surface/stride limits apply. The API owns no persistent resources
and returns UNSUPPORTED with NO_GRAPHICS. Invalid inputs leave pixels unchanged.

Painting uses temporary RGBA and mask buffers. Native allocation errors return
OUT_OF_MEMORY; a single gradient's destination is unchanged when drawing fails
before composition. A whole scene remains nontransactional. The scene renderer
checks two RGBA buffer sizes against a configured memory budget; complete
aggregate accounting of native/backend allocations is still open.

## Русский

В DefineShape/2/3 поддержаны линейные и радиальные градиенты с матрицей и
контрольными точками RGB/RGBA. Для этих тегов спецификация требует нулевые
режимы распространения градиента/интерполяции и 1..8 точек. Повреждённые записи, неверный
порядок позиций и лишние байты отвергаются. Ресурсы сохраняются как обычный
управляемый граф и переносятся вместе со сценой через очередь, файл и снимок.

Матрица градиента независима от контура и объединяется с преобразованием сцены.
Сначала интерполируется прямой RGBA, затем применяется итоговое преобразование
цвета; это важно, когда компоненты ограничиваются диапазоном 0..255. Сглаженная
маска ограничивает заливку, сохраняя отверстия. AGG/Cairo используют общую
цветовую рампу. Вне рампы сохраняются крайние цвета; при одинаковых позициях
на самой границе выбирается последняя точка.

Нативный C89 API работает с обычными редактируемыми путями и допускает до
15 точек. Матрица переводит нормализованный квадрат -1..1 в пиксели; ограничения
на матрицы, цвета, поверхности и временную память приведены выше. Описание
градиента заимствует массив точек только на время вызова. При NO_GRAPHICS
возвращается UNSUPPORTED. Ошибочный вход или отказ памяти одной заливки не
должен менять поверхность до композиции; вся сцена атомарной не является.

## Tests / Тесты

`tests/gradient.c` is compiled as strict C89. It checks ramps, reflection/rotation,
radial sampling, duplicate ratios, post-interpolation color clamping, straight-alpha
source-over and invalid input without pixel mutation. `tests/swf_gradient.cpp`
checks both types in all three shape tags, queue/file/snapshot transport, GC,
truncated prefixes, reserved modes/counts and unsorted stops. Its pixel assertions
run in AGG/Cairo; NO_GRAPHICS checks decoding/transport and the explicit unsupported
draw result. `tests/swf_gradient_allocation.cpp` injects each allocation boundary
in decoding, managed loading and painting, checks output publication, collection
and unchanged destination after a failed paint.

The [C89 tutorial](../examples/embedding/gradients/README.md) explains every code
line, matrix conventions, borrowed stop lifetime, pixel ownership and errors.

## Remaining / Остаётся

MovieClip.beginGradientFill, the complete ActionScript geometry/Matrix API,
Shape4/LINESTYLE2, focal gradients, repeat/reflect spread, linear-RGB interpolation,
bitmap fills, masks across scene layers, filters/blend modes, embedded text/fonts
and multimedia remain open. Native vector gradients do not certify these APIs.
Full historical-player, allocation-accounting and target-platform acceptance are
still required. This implementation does not close any roadmap stage.

ActionScript API градиентов и геометрии, режимы Shape4, bitmap-заливки, маски,
фильтры, текст/шрифты и мультимедиа пока не завершены. Нужны эталонные SWF и
сверка с Flash Player, общий учёт памяти и запуски на целевых платформах.

Reference: [Adobe SWF specification, Gradients](https://open-flash.github.io/mirrors/swf-spec-19.pdf).

## Validation v0.30 / Проверки v0.30

Strict Linux C89/C++98 profiles passed: headless 183/183, bundled AGG 193/193,
bundled Cairo 193/193, portable/no-OS 174/174. Headless and AGG needed successful
reruns after workspace executable-permission failures; logs are kept outside Git.
The final affected gradient/transport/color tests also passed in all four profiles.
The sanitizer driver completed 180 tests/examples after the final test additions.
Heap/global ASan and UBSan were enabled; all ASan stack checks were explicitly
disabled for the workspace unwind environment, LeakSanitizer was disabled because
/proc is unavailable, and container exception tests use UBSan only. Comment audits
covered 245 own-source files with no missing function/member/tutorial-line comments
and no missing direct GC tags. Complete Flash/platform acceptance remains open.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_Gradient` | struct | [include/flashvm/gradient.h:30](../include/flashvm/gradient.h#L30) |
| `fvm_path_draw_gradient` | function | [include/flashvm/gradient.h:44](../include/flashvm/gradient.h#L44) |
<!-- source-links:end -->
