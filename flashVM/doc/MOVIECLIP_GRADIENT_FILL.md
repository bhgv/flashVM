# MovieClip.beginGradientFill / Градиентная заливка ActionScript

Current color-policy extension v0.1183+: [linearRGB fills and strokes](LINEAR_RGB_GRADIENTS.md) / [интерполяция linearRGB](LINEAR_RGB_GRADIENTS.ru.md). Earlier RGB-only limits describe historical checkpoints. DefineShape4/LineStyle2 and nonzero focal remain open; all nine stages remain open.


Current stroke extension v0.1169+: [lineGradientStyle and C89 strokes](GRADIENT_STROKES.md) / [градиентные обводки](GRADIENT_STROKES.ru.md). Earlier lineGradientStyle limitations below describe the historical checkpoint; current RGB/zero-focal stroke support does not close full Flash line semantics or any roadmap stage.


Current extension: [pad, reflect and repeat](GRADIENT_SPREAD.md) / [режимы продолжения](GRADIENT_SPREAD.ru.md). Earlier PAD-only descriptions below document the original implementation; non-pad modes are now available through the additive API and SWF 8 beginGradientFill. Shape parsing limits remain unchanged.


Development checkpoint v0.31. Roadmap stages 3 and 4 remain open.

MovieClip.beginGradientFill is now available through the standard prototype and
AVM1 CallMethod. Linear and centered radial fills use pad spread, ordinary RGB
interpolation and up to 15 ordered stops. Colors/alphas/ratios must have equal
lengths; array-like properties use ordinary lookup and coercion. Ratios are
converted to byte positions and alphas to byte opacity. RGB values truncate toward
zero and wrap modulo 24 bits, including negative fractions. The matrix can be a
legacy `{matrixType:"box",x,y,w,h,r}` object or an object exposing the Matrix
fields a/b/c/d/tx/ty. Box rotation uses radians and independent horizontal/vertical
scales. The flash.geom.Matrix class itself is not yet installed by this checkpoint.

All stops and matrix components are copied before publication; later mutation of
the arguments does not change a chosen fill. New drawing paths retain immutable
descriptors, so endFill/beginFill can release the current ramp while completed
paths continue to render. lineStyle preserves the fill ramp. clear resets fill,
stroke and current ramp state while leaving child display objects in place.
No arguments, or an undefined first argument, disable the current fill.
Malformed types, lengths, ratios and singular/nonfinite matrix arguments leave
the old style unchanged and return undefined. Unsupported spread/interpolation,
nonzero radial focal ratios and legacy nine-field matrices return UNSUPPORTED.
Exceptions from user coercion propagate; callback side effects are not rolled back.

Private drawing-state publication is atomic under native allocation failure.
The native-call barrier prevents collector reentry while temporary graph edges
are held. Completed geometry and the current descriptor are cloned together by
duplicateMovieClip, preserving their aliases within the new independent graph.
Queue/file transport restores both the drawing state and its callable method.
The new callback is appended to the existing stable identifier list; prior callback
identifiers, ABI 3 and graph format 7 remain unchanged.

The host subtree renderer now composes ancestor color transformations as well
as its existing world matrix. A gradient's final RGBA therefore includes its
parents' alpha/color terms even when the host renders that child directly.
Zero-thickness ActionScript strokes retain the device-pixel hairline behavior.
These changes do not certify all style transitions, visibility/input semantics
or historical Flash version differences.

## Русский

Метод подключён к стандартному прототипу MovieClip и проверен через AVM1
CallMethod. Поддерживаются linear/radial, pad, RGB и до 15 упорядоченных точек.
Массивы цветов, прозрачности и позиций имеют одинаковую длину. Матрица задаётся
legacy box-объектом либо полями a/b/c/d/tx/ty; отдельный класс flash.geom.Matrix
ещё не установлен. Поворот box задаётся в радианах с независимыми масштабами осей.

Параметры копируются: изменение исходных массивов и матрицы не меняет рампу.
Готовые контуры удерживают свои описания заливки после endFill/beginFill.
lineStyle сохраняет градиент; clear сбрасывает стили рисования, не удаляя детей.
Вызов без аргументов или с undefined отключает текущую заливку. Неверные
основные параметры сохраняют прежний стиль и возвращают undefined. Пока
неподдерживаемые режимы и девятиполевая матрица возвращают UNSUPPORTED.
Побочные эффекты пользовательских преобразований не откатываются; их исключения
передаются вызывающему коду.

При отказе выделения памяти новый стиль не публикуется частично. Нативный барьер
защищает временные связи. Дублирование копирует рампу вместе с активным контуром,
сохраняя внутренние общие ссылки и независимость от исходного клипа. Перенос
через очередь/файл восстанавливает данные и работоспособный метод. Старые коды
обратных вызовов, ABI и формат графа не изменены.

При отрисовке поддерева учитываются цветовые преобразования предков. Нулевая
толщина линии означает один пиксель устройства. Полная семантика Flash, включая
сложные переходы стилей и различия версий, всё ещё требует сверки.

## Tests / Тесты

`tests/movieclip_gradient.cpp` checks box/Matrix-shaped arguments, nonsquare box
rotation, copied inputs, real AVM1 dispatch, current/completed descriptor lifetime,
duplicate aliasing, lineStyle/clear/no-argument behavior, negative-fraction colors,
invalid/unsupported options, queue/file restoration and invocation after import,
GC and parent-alpha subtree rendering. The test explicitly restores deleted
array elements after length truncation: growing length cannot restore deleted data.
Pixel assertions execute in AGG and Cairo; headless validates state/transport.
`tests/movieclip_gradient_allocation.cpp` injects every native allocation boundary
until the style update succeeds and confirms that the previous current/active
edges and rendered pixels survive failures and subsequent collection.
The [AS2 tutorial](../examples/embedding/gradients/example_as2.as) explains every
code line, argument copying and descriptor lifetime.

## Remaining / Остаётся

Repeat/reflect, linearRGB, focal gradients, lineGradientStyle, legacy nine-field
matrix semantics, the full geometry/Matrix API, bitmap fills, masks, filters,
embedded text/fonts and media are open. SWF-version gating, exact coercion and
fixed/twip quantization, all drawing-style transitions and reference-player tests
are also open. Existing renderer memory/coordinate limits still apply; a whole
scene is not a transaction. No stage or target-platform acceptance is closed.

Остальные режимы градиентов, lineGradientStyle, геометрические классы, bitmap,
маски, фильтры, текст/шрифты и мультимедиа остаются открытыми. Нужна полная
сверка преобразований, квантования, порядка исполнения и различий версий.

Reference: [Adobe AS2 MovieClip reference](https://open-flash.github.io/mirrors/as2-language-reference/MovieClip.html).
Independent cross-check: [Ruffle gradient arguments](https://github.com/ruffle-rs/ruffle/blob/master/core/src/avm1/globals/movie_clip.rs)
and [matrix conventions](https://github.com/ruffle-rs/ruffle/blob/master/render/src/matrix.rs).
No upstream implementation code was copied.

## Validation v0.31 / Проверки v0.31

Strict Linux C89/C++98 profiles passed: headless 185/185, bundled AGG 195/195,
bundled Cairo 195/195 and portable/no-OS 176/176 after correcting the test's
array-restoration setup. The sanitizer driver completed 182 tests/examples using
the corrected test source. Heap/global ASan and UBSan were enabled; ASan stack
checks were explicitly disabled for the workspace unwind environment,
LeakSanitizer was disabled because /proc is unavailable, and container exception
tests use UBSan only. Audits covered 247 first-party source files with no missing
function/member/tutorial-line comments and no missing direct GC tags. Logs and
generated audit results remain outside Git. No roadmap stage is closed.
