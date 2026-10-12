# BitmapData.draw

## Русский

v0.50 добавляет draw(source, matrix, colorTransform, blendMode, clipRect,
smooth). Источник — live BitmapData или MovieClip. Matrix и ColorTransform
читаются через публичные поля и копируются до рендеринга; null/undefined
оставляют identity. При BitmapData smooth выбирает bilinear или nearest;
при MovieClip действует собственный smoothing bitmap-заливок потомков.

MovieClip рисуется в координатах своей библиотеки: сценическая матрица,
цвет/alpha, visibility и pixel snapping корневого источника игнорируются;
матрицы, цвет и visibility потомков сохраняются. Игнорирование root visibility
— поведение текущей реализации, требующее дополнительной Flash-сверки.
Переданный matrix/color применяется вместо root transforms. Методы рендеринга
обычной сцены сохраняют прежнюю семантику world/ancestor transforms.

Источник сначала растеризуется в отдельный прозрачный RGBA буфер. Затем
без дальнейших аллокаций/callbacks смешивается с destination. Это сохраняет
source==destination и предотвращает частичную запись при OOM в любом paint
или потомке. Получатели opaque всегда сохраняют alpha=255. Если getter
во время рендеринга dispose-ит destination, публикация отклоняется после
проверки storage; native barrier сохраняет управляемые объекты живыми.
Побочные эффекты пользовательских getters не откатываются.

В v0.51 реализованы normal/layer, multiply, screen, lighten/darken, difference,
add/subtract, invert, overlay/hardlight, alpha и erase. Сепарабельный RGB результат
композируется с alpha source-over; компоненты округляются к ближайшему
байту. При layer источник уже flattened в отдельный буфер. Имена и integer
IDs 1..14 распознаются. Alpha/erase изменяют только alpha прозрачного
получателя; opaque получатель не меняется в этих двух режимах.
Для alpha одновременно с source собирается отдельное геометрическое
coverage, не повторяя getters: Da × (1 − coverage + Sa). Erase использует
Da × (1 − Sa). RGB сохраняется до обнуления alpha, затем очищается.
Область вне геометрии не очищается; учитываются AA, отверстия и stroke-only
контуры. Полная Flash-квантизация, group blending и взаимодействие
с masks/filters не подтверждены.

clipRect — destination rectangle; сейчас отбираются pixel centers внутри
него, без дополнительного fractional clip coverage. Нулевой/почти нулевой
determinant (<1e-20) и пустой clip дают no-op. Числовые bounds: matrix ±1e6,
color coefficients/offsets ±65536, clip fields ±2e6. Действуют ограничения
существующего raster path backend (включая координаты ±4096). Scratch RGBA
проверяется против memory_limit/3 (memory_limit/4 для alpha coverage); это не полная квота всех VM allocations.

bitmap_draw.cpp проверяет nearest/bilinear, reflection, clipping, self alias,
opaque alpha, фиксированные blend vectors, normalized MovieClip root и
child poses, внешний ColorTransform, лимит, disposed receiver через getter,
GC и queue transfer callback/source без standard_install в receiver.
bitmap_draw_allocation.cpp инъецирует каждую границу new/new[] для bitmap
и многослойного clip, проверяет destination rollback и баланс аллокаций после
уничтожения VM. bitmap_draw_coverage.cpp проверяет прозрачные/полупрозрачные
bitmap/clip, self alias, AA edges, holes/strokes, clipping и single getter
read. Профили: AGG, Cairo, headless, portable и heap/global
ASan+UBSan; stack ASan/LeakSanitizer выключены в специальном профиле.
Native callback ID 1540 сохраняет прежние bitmap IDs.

Открыто: полная alpha/erase Flash-сверка, дробный clipping и точное Flash округление,
child blend modes/group effects, masks/filters и остальные BitmapData API,
полный текст, шрифты, звук и видео. Этап 4 ROADMAP.md не завершён.

## English

v0.50 adds draw from live BitmapData or MovieClip with copied public
Matrix/ColorTransform fields, normal defaults, destination clipping and
bitmap nearest/bilinear sampling. MovieClip source rendering ignores its
root stage pose/color/alpha/visibility/snapping while retaining descendant
poses, colors and visibility. Root visibility behavior still needs reference
conformance. Ordinary scene world/ancestor rendering remains unchanged.

The complete source is rendered into separate transparent RGBA before
allocation-free destination publication. Source aliases are safe and failed
paint/child allocations cannot publish a partial image. Disposed destination
storage is rechecked after reentrant getters; getter side effects themselves
are not rolled back. Opaque receivers retain alpha=255.

v0.51 implements all fourteen recognized draw modes: normal/layer,
multiply/screen, lighten/darken, difference, add/subtract, invert,
overlay/hardlight, alpha and erase. Separable RGB blending uses source-over
alpha and nearest-byte rounding. Alpha uses Da × (1 − coverage + Sa), with
geometry coverage collected during the same scene traversal without repeated
getters; erase uses Da × (1 − Sa). Only transparent receiver alpha changes,
with RGB retained until alpha becomes zero. Outside geometry is preserved,
including AA edges, holes and strokes. Opaque alpha/erase targets are unchanged
in this model. Exact Flash quantization, group blending and masks/filter
interactions are still unverified.

Clip selection currently uses destination pixel centers without fractional
clip coverage. Empty clips and determinant magnitude below 1e-20 are no-ops.
Explicit numeric bounds and existing raster path limits apply. Scratch bytes
are checked against memory_limit/3 (memory_limit/4 for alpha coverage),
without claiming complete VM peak quotas.
Tests cover sampling/transforms/clipping, self aliases, blend vectors, root
normalization/children, color, budgets, reentrant disposal, GC/queue transport
and new/new[] failure rollback/balance. Coverage tests include transparent
pixels, self aliases, AA edges, holes/strokes and single getter evaluation.
AGG/Cairo, disabled-graphics profiles
and heap/global ASan+UBSan are checked. Callback ID 1540 preserves older IDs.
Remaining coverage, conformance, filters/bitmap APIs, text and media keep
stage 4 open.
