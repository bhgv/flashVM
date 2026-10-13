# Реестр выполнения / Compatibility status

Ориентир: [ROADMAP.md](ROADMAP.md). Базовый реестр уточняется по мере сравнения
с нормативными SWF/AS API; это ещё не полный каталог совместимости.
Статусы: отсутствует / реализовано / проверено / завершено. «Проверено» относится
к перечисленным сценариям, а не ко всем возможным входам или платформам.

| Пункт | Статус | Подтверждение и оставшаяся работа |
| --- | --- | --- |
| 1: значения String с NUL и суррогатами | Проверено для базовых сценариев v0.16 | standard_utf16.c, standard_string.c, string_wire.c; ABI 3, граф 4. См. UTF16_STRINGS.md. |
| 1: Unicode upper/lower и CASEINSENSITIVE | Проверено для выбранных сценариев v0.16 | Кириллица, расширения, финальная сигма; Unicode 3.2. Полная сверка с историческим Flash открыта. |
| 1: Object, Array и ключи | Реализовано частично | Watch/перечисление есть; generic Array, наследование индексов и registerClass проверены в сценариях v0.17 (GENERIC_ARRAYS.md). Полноценные строковые ключи, пределы длины и полная сверка открыты. |
| 2: AVM1 | Реализовано частично | Регистры Function2, основные preloads и aliasing именованных arguments проверены в v0.20 (AVM1_FUNCTION2.md). Extends/ImplementsOp/CastOp и транзитивные интерфейсы проверены в v0.22 (AVM1_INHERITANCE.md). Основные вызовы super и перенос фасадов проверены в v0.23 (AVM1_SUPER.md); числовые super-свойства, регистровые aliases и полная сверка областей/версий ещё открыты. |
| 3: MovieClip и SWF-сцена | Реализовано частично | В v0.24 проверен импорт корневых DoAction/FrameLabel из FWS/CWS по кадрам и перенос через очередь/файл (SWF_MOVIECLIP_TIMELINE.md). В v0.25 проверены DefineSprite/ExportAssets и attachMovie с initObject/registerClass и переносом библиотеки (SWF_SPRITE_RESOURCES.md). В v0.26 проверено duplicateMovieClip с независимым рисунком, конструкторами и переносом (MOVIECLIP_DUPLICATE.md). В v0.27 проверены знаковые глубины и совместимость прежних снимков (MOVIECLIP_DEPTHS.md). В v0.28 проверены размещение/перемещение/удаление спрайтов, реконструкция кадров, матрицы и цвет, перенос и отказы памяти (SWF_SCENE_CONTROLS.md). В v0.29 подключены сплошные DefineShape/2/3: контуры, кривые, отверстия, замена геометрии, перенос и отказы памяти (SWF_SOLID_SHAPES.md). Полный display list и остальные ресурсы открыты. |
| 4: линейные/радиальные градиенты | Проверено для сценариев v0.30 | SWF Shape1/2/3 и C89 API: матрицы, цвет, перенос и отказы памяти (SWF_GRADIENTS.md). В v0.31 проверен beginGradientFill pad/RGB, копирование аргументов, AVM1, дублирование/перенос и OOM (MOVIECLIP_GRADIENT_FILL.md). Shape4, другие режимы, полная геометрия и остальная графика/мультимедиа открыты. |
| 4: flash.geom.Point/Matrix | Проверено для числовых сценариев v0.32 | standard_geometry.cpp и geometry_allocation.cpp: все публичные методы, частичные конструкторы, матрицы, очередь/файл, GC и отказы памяти. В v0.33 проверены строки/NUL и отдельные getter/setter overrides (geometry_dispatch.cpp). Полная семантика getter/Math overrides и остальные классы открыты (GEOMETRY_CLASSES.md). |
| 4: flash.geom.Rectangle | Проверено для сценариев v0.34 | standard_rectangle.cpp: методы, производные свойства, границы, AVM1 NewMethod, queue/file/loopback TCP и GC. rectangle_allocation.cpp: отказы фабрик. Полная семантика coercion/Math и удалённые процессы открыты (RECTANGLE.md). |
| 4: flash.geom.ColorTransform | Проверено для сценариев v0.35 | standard_color_transform.cpp: каналы/rgb/concat, копия, queue/file/TCP, GC и валидация состояния. color_transform_allocation.cpp: копия и десериализация с OOM. Применение через Transform и полная сверка открыты (COLOR_TRANSFORM.md). |
| 3–4: Transform и MovieClip.transform | Проверено для сценариев v0.36 | standard_transform.cpp: матрицы/цвет/границы, snapshots, GC, queue/file/TCP с общими ссылками. transform_allocation.cpp: атомарная запись цвета и OOM фабрик. Полные pixelBounds, удаление клипов и точная квантизация открыты (TRANSFORM.md). |
| 4–9 | Открыты | См. ROADMAP.md и прежние контрольные точки; завершение не подтверждено. |

No roadmap stage is closed. Update this inventory with concrete tests, profiles
and limitations when implementing a remaining item. This initial inventory is
not yet a complete SWF tag/action/AS API compatibility catalogue.

## v0.37 BitmapData subset / Частичная реализация BitmapData

Native pixel storage, size/rectangle accessors, clone/dispose, pixel access,
fillRect, scroll and colorTransform are implemented. Queue/file graph snapshots
retain factories and validate binary state. See [BITMAP_DATA.md](BITMAP_DATA.md)
for test names and remaining conformance/graphics limitations. Stage 4 remains
open; no full BitmapData, rendering or media compatibility is claimed.

Реализован перечисленный набор BitmapData; остаются premultiplied rounding,
остальные операции, интеграция сцены/заливок и полная приёмка.

## v0.38 BitmapData.copyChannel

Implemented selected-channel copying with joint source/destination clipping,
overlap snapshots, opaque-alpha preservation and allocation-before-mutation.
standard_bitmap and bitmap_allocation cover the operation.
Реализировано копирование отдельного канала с отсечением и независимым снимком;
полная совместимость BitmapData и этап 4 остаются открытыми.

## v0.39 Number power-of-two formatting / Форматирование Number

Exact finite fractions and large integers in bases 2/4/8/16/32 are implemented
and covered by standard_number, including extreme binary64 values and queue
transport. [NUMBER_RADIX.md](NUMBER_RADIX.md) records the remaining limits.
Остальные дробные основания и историческая сверка открыты; этап 1 не закрыт.

## v0.40 Numeric AVM1 actions

Increment/decrement, remainder, bitwise operations/shifts and Greater are
implemented; avm1_numeric_operations verifies boundaries, coercion order and
error recovery. See AVM1_NUMERIC_ACTIONS.md. Полная семантика AVM1 открыта.

## v0.41 Vector MovieClip shape hit testing

MovieClip.hitTest point/shape queries and the backend-independent C89 path query
are implemented with documented subdivision/boundary tolerances. path_hit and
movieclip_shape_hit cover fill rules, curves, strokes, transforms, queue/file and
GC. See MOVIECLIP_SHAPE_HIT.md. Маски/bitmap/text и полный этап 3 остаются открыты.

## v0.42 BitmapData.copyPixels

Implemented clipped pixel copies, alpha masks, mergeAlpha and opaque source-over
composition with source/mask snapshots before mutation. bitmap_copy_pixels and
bitmap_allocation cover transport, overlap and OOM. See BITMAP_DATA.md for
premultiplied/rounding limitations. Полный этап 4 остаётся открытым.

## v0.43 MovieClip bitmap fills / Bitmap-заливки

beginBitmapFill and the C89 bitmap painter implement copied affine matrices,
live texture edges, repeat/edge extension, nearest/bilinear sampling, inherited
color transforms and graph transfer. Duplication shares texture identity while
copying geometry. Native path RAII and marked AGG exception-safety corrections
handle injected allocation failures. See MOVIECLIP_BITMAP_FILL.md for named tests
and remaining resource/filter/text/media limits. Полный этап 4 остаётся открытым.

## v0.44 Hidden bitmap attachments

attachBitmap implements live hidden display nodes, depth replacement, render-only
auto/always/never snapping and smoothing. Child detachment is staged before
publication to preserve old nodes on OOM. movieclip_attach_bitmap and its
allocation suite cover pixels, visibility through depth APIs, queue/file, GC
and imported methods. See MOVIECLIP_ATTACH_BITMAP.md for remaining conformance
limits. SWF resources, complete graphics/text/media and stage 4 remain open.

## v0.45: lossless SWF bitmaps

Проверены DefineBitsLossless/2, форматы RGB555/XRGB/ARGB и палитры, bitmap-заливки Shape1/2/3, GC/перенос и отказы памяти. Ограничения и RU/EN описание: [SWF_LOSSLESS_BITMAPS.md](SWF_LOSSLESS_BITMAPS.md). Этап 4 открыт.

Lossless tags and bitmap shape fills have format, rendering, GC/transport and allocation-failure coverage. JPEG resources and the rest of stage 4 remain open.

## v0.46: JPEG2/3

JPEG2/3: проверяются RGB/grayscale, отдельный alpha, границы потоков, пиксели, GC/очередь и отказы C++ памяти. RU/EN ограничения: [SWF_JPEG_BITMAPS.md](SWF_JPEG_BITMAPS.md). Пункт 4 открыт.

JPEG2/3 has binary, pixel, GC/queue and C++ allocation-failure coverage; legacy tables, PNG/GIF and CMYK remain open.

## v0.47: embedded PNG/GIF

Проверены PNG palette/tRNS, gray/16-bit/Adam7 и full-canvas GIF89a с palettes/interlace, ошибки потоков, pixels/GC/очередь и failed C++ copies. [SWF_PNG_GIF_BITMAPS.md](SWF_PNG_GIF_BITMAPS.md) описывает RU/EN ограничения. Пункт 4 открыт.

SWF 8 PNG and single-frame GIF layouts have pixel/transport and malformed-input coverage. Partial GIF canvases and exact Flash color behavior remain open.

## v0.48: DefineBits/JPEGTables

Проверен bounded subset общего JPEG dictionary и placement SWF 1..8, framing/truncation, pixels/GC/очередь и отказы памяти. RU/EN: [SWF_JPEG_TABLES.md](SWF_JPEG_TABLES.md). Полная JPEG/Flash сверка и этап 4 открыты.

Shared table JPEG and SWF 1..8 placement have binary/pixel/GC/transport and allocation-failure coverage; full reference conformance remains open.

## v0.49: BitmapData.loadBitmap

Static loadBitmap: независимые копии exported bitmap, AVM1/current/global scope, NUL/missing linkage, GC/OOM и queue/file/TCP. [BITMAP_LOAD_RESOURCE.md](BITMAP_LOAD_RESOURCE.md) содержит RU/EN ограничения; loaded-library/closure scope и этап 4 открыты.

Static resource copies and transport are checked; full library scope and remaining graphics/media remain open.

## v0.50: BitmapData.draw

Проверенный subset draw BitmapData/MovieClip: transforms, color, clipping, nearest/bilinear, 12 blend modes, GC/queue и allocation rollback/balance. [BITMAP_DRAW.md](BITMAP_DRAW.md) описывает RU/EN ограничения; alpha/erase, masks/group effects и этап 4 открыты.

Transactional drawing has fixed-vector/scene and allocation-failure coverage; complete blend/Flash conformance remains open.

## v0.51: draw alpha/erase

Проверены alpha/erase с geometry coverage, AA/holes/strokes, outside preservation, self alias, single getter read и OOM rollback/balance. [BITMAP_DRAW.md](BITMAP_DRAW.md) описывает текущую модель всех 14 draw IDs и ограничения. Полная Flash/group/filter сверка и этап 4 открыты.

Alpha/erase coverage has explicit pixel and allocation-failure coverage; all draw mode IDs have an implemented model, with reference conformance still open.

## v0.52: floodFill

Scanline floodFill проверен на ARGB/4-connectivity, bounded scratch, coercion/disposal, GC/queue и later-stack OOM rollback/balance. [BITMAP_FLOOD_FILL.md](BITMAP_FLOOD_FILL.md) содержит RU/EN ограничения; полная Flash-сверка и этап 4 открыты.

Backend-independent connected fills have bounded-memory and failure-path coverage; full Flash conformance remains open.

## v0.53: getColorBoundsRect

Проверены ARGB masks/matching, boundaries/default/inverse/empty cases, captured Rectangle и queue/GC/OOM/balance. RU/EN: [BITMAP_COLOR_BOUNDS.md](BITMAP_COLOR_BOUNDS.md). Premultiplied/Flash сверка и этап 4 открыты.

Masked color bounds and transported result factories have explicit edge/failure coverage; stage 4 remains open.

## v0.54: threshold

Проверены unsigned masked comparisons, copy/defaults/opaque alpha, shared clipping/snapshot, self alias, GC/queue и OOM/balance. RU/EN: [BITMAP_THRESHOLD.md](BITMAP_THRESHOLD.md). Fractional/premultiplied/Flash сверка и этап 4 открыты.

Threshold and aligned region snapshots have explicit edge/transport/failure coverage; complete Flash conformance remains open.

## v0.55: merge

Per-channel merge: проверены weights/rounding/clamps, opaque alpha, overlap, budgets, GC/queue и OOM/balance. RU/EN: [BITMAP_MERGE.md](BITMAP_MERGE.md). Полная Flash/premultiplied сверка и этап 4 открыты.

Channel merging has fixed-vector/alias/transport/failure coverage; complete reference conformance remains open.

## v0.56: paletteMap

Проверены sparse palettes, identity channels, modulo-32 contributions, overlap, quota, disposal during getters, GC/queue and OOM/balance. RU/EN: [BITMAP_PALETTE_MAP.md](BITMAP_PALETTE_MAP.md). Полная Flash-сверка и пункт 4 открыты.

## v0.57: BitmapData.hitTest

Проверены три overloads, пороги alpha, границы и integer wrap, disposal/getters, queue/file, GC и injected OOM/balance. RU/EN: [BITMAP_HIT_TEST.md](BITMAP_HIT_TEST.md). Полная историческая Flash-сверка открыта.

## v0.58: BitmapFilter / ColorMatrixFilter

Проверено для color-matrix bitmap subset: inheritance, clone/matrix copies, aligned applyFilter, unchanged generateFilterRect bounds, malformed state, queue/file/TCP, GC и OOM/balance. RU/EN: [COLOR_MATRIX_FILTER.md](COLOR_MATRIX_FILTER.md). Другие фильтры, MovieClip.filters, SWF filter lists и full Flash conformance открыты.

## v0.59: ConvolutionFilter

Проверено для bitmap convolution subset: defaults/options, private matrix copies/clone, edge clamp/replacement, halo sampling, overlap, transport, invalid state и OOM/balance. RU/EN: [CONVOLUTION_FILTER.md](CONVOLUTION_FILTER.md). Flash rounding/premultiplied/coercion conformance, MovieClip filters и полный этап 4 открыты.

## v0.60: BitmapData.noise

Проверено для deterministic noise subset: signed/extreme seeds, Schrage arithmetic against independent double oracle, masks/grayscale, ranges, opaque alpha, quota/OOM, getter disposal, queue/file/TCP и GC. RU/EN: [BITMAP_NOISE.md](BITMAP_NOISE.md). Premultiplied alpha quantization и полная историческая Flash-сверка открыты.

## v0.61: opaque noise sampling correction

Opaque noise skips alpha RNG samples, matching the current Ruffle implementation. All 16 masks are checked in color/grayscale; transparent vectors and OOM/transport coverage remain. Историческая Flash-квантизация и полная приёмка открыты (BITMAP_NOISE.md).

## v0.62: BitmapData.perlinNoise

Реализовано для bounded procedural subset: SVG gradient turbulence/fractal sums, channel packing/grayscale, public offsets, stitch boundary, atomic buffers и callback 1550. Проверки: bitmap_perlin.cpp / bitmap_perlin_allocation.cpp. RU/EN: [BITMAP_PERLIN.md](BITMAP_PERLIN.md). Полная Flash/premultiplied/version/offset conformance и пункт 4 открыты.
