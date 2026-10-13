# Реестр выполнения / Compatibility status

Ориентир: [ROADMAP.md](ROADMAP.md). Базовый реестр уточняется по мере сравнения
с нормативными SWF/AS API; это ещё не полный каталог совместимости.
Статусы: отсутствует / реализовано / проверено / завершено. «Проверено» относится
к перечисленным сценариям, а не ко всем возможным входам или платформам.

| Пункт | Статус | Подтверждение и оставшаяся работа |
| --- | --- | --- |
| 1: значения String с NUL и суррогатами | Проверено для базовых сценариев v0.16 | standard_utf16.c, standard_string.c, string_wire.c; ABI 3, граф 4. См. UTF16_STRINGS.md. |
| 1: Unicode upper/lower и CASEINSENSITIVE | Проверено для выбранных сценариев v0.16 | Кириллица, расширения, финальная сигма; Unicode 3.2. Полная сверка с историческим Flash открыта. |
| 1: Object, Array и ключи | Реализовано частично | Watch/перечисление есть; generic Array, наследование индексов и registerClass проверены в сценариях v0.17 (GENERIC_ARRAYS.md). Counted property get/set/flags/delete, AVM1 member operations и graph 8 проверены в v0.81 (COUNTED_PROPERTY_KEYS.md). Counted calls/registration, пределы длины и полная сверка открыты. |
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

## v0.63 — Local file channel debugging

Реализованы freeze чтения и indexed non-consuming peek локальных файлов; GC/циклы, блокирующие читатели и malformed input проверены в file_channel_debug. Remote inspection и file state snapshot отсутствуют. См. FILE_CHANNEL_DEBUG.md.

Local file freeze and indexed inspection are implemented; remote inspection and file state snapshots remain absent. Full debugger acceptance remains open.

## v0.64 — Bitmap dissolve model

Реализована детерминированная модель pixelDissolve: continuation, clipped copy/fill, atomic staging, native callback transport. Полная Flash-семантика порядка/seed/count/premultiplied alpha **не подтверждена**; метод не завершён. См. BITMAP_DISSOLVE.md.

Deterministic dissolve API implemented; exact historical Flash behavior is not certified and remains open.

## v0.65 — Local file state snapshots

Non-consuming local file pending/frozen/closed inspection implemented; malformed/read failures preserve output and cursor. Проверки в file_channel_debug; remote snapshot и полная приёмка стадии 7 открыты. См. FILE_CHANNEL_DEBUG.md.

## v0.66 — Managed fractional box blur

BlurFilter constructor/properties/clone, bitmap application and expanded bounds implemented with staged premultiplied planes and private graph validation. Полная Flash/scene совместимость остаётся открытой. Проверки: blur_filter, blur_filter_allocation; BLUR_FILTER.md.

## v0.67 — Remote channel debugger controls

Compatible broker queue/file freeze/state implemented with validated controls and bounded file scans. Проверки: channel_debug_network, channel_debug_protocol, file_channel_debug. Remote payload inspection, distributed wait/deadlock graphs and complete acceptance remain open; REMOTE_CHANNEL_DEBUG.md.

## v0.68 — Native string result lifetime

Context-owned snapshots retain native string results across userdata finalization, with counted-byte arena descriptors and atomic indexes. Проверки: gc_string_pool, gc_native_string_result, gc_native_string_allocation. Arena strings remain retained to context teardown; traced reclamation and full GC acceptance remain open (GC_STRING_STORAGE.md).

## v0.80 — Shared-proxy/local mutex cycle

Проверено / tested: locally observed mixed wait cycle between one retained TCP
proxy shared by two OS workers and one local mutex; owner/waiter locations,
endpoint, cancellation cleanup and reuse. Regression: `mutex_mixed_deadlock.c`.
Default AGG regression suite: 242/242 (examples disabled); mixed test 100 repeats;
ASan/UBSan targeted 5/5, leak scanning unavailable; portable excludes live TCP.
Separate proxy/process owners and full distributed deadlock detection remain
open. RU/EN scope and validation: [REMOTE_MUTEX_DEBUG.md](REMOTE_MUTEX_DEBUG.md).

## v0.81 — Counted property keys

Проверено / tested: counted UTF-8/WTF-8 names with embedded NUL and equivalent
UTF-16 scalar/pair representations, property attributes, lookup, enumeration,
watch and AVM1 member access; graph 8 queue/file/TCP with malformed-input and
allocation rollback checks. Default suite 244/244; portable 2/2; headless
ASan/UBSan 5/5 with stack/leak exclusions. Stage 1 remains open; counted calls,
native registrations and variable names are not covered yet.
RU/EN: [COUNTED_PROPERTY_KEYS.md](COUNTED_PROPERTY_KEYS.md).

## v0.82 — Counted native member dispatch

Проверено / tested: native member/accessor registration and invocation with NUL
and canonical UTF-16 keys, retained captures, inherited receivers, cached Function
identity and AVM1 CallMethod/NewMethod path. Size-aware table property queries
added. Default 245/245; headless ASan/UBSan targeted 4/4; portable new test 1/1.
Global/function variable names and full stage-1 conformance remain open.
RU/EN and regression scope: [COUNTED_PROPERTY_KEYS.md](COUNTED_PROPERTY_KEYS.md).

## v0.83 — Counted global and lexical names

Проверено / tested: global get/set/native registration/call, AVM1 local declaration,
variable access/assignment and function lookup with full canonical names. Missing
NUL-suffixed native constructors cannot invoke a terminated prefix. Legacy graph
keys normalize during restore; graph version remains 8 and public layout ABI 3.
AGG 246/246; Cairo Release 246/246; ASan/UBSan targeted 5/5 with stack/leak
exclusions; portable targeted 2/2. Stages 1–9 remain open. RU/EN behavior and
remaining scope: [COUNTED_PROPERTY_KEYS.md](COUNTED_PROPERTY_KEYS.md).
Final acceptance gates: [RELEASE_ACCEPTANCE.md](RELEASE_ACCEPTANCE.md).

## v0.84 — Counted constructor and stack APIs

Проверено / tested: canonical counted native construction/registration,
AVM1 NewObject fallback, rooted stack/Lua-style registration and calls,
counted field/global/string auxiliary helpers. Regression:
native_counted_construction. AGG 247/247, Cairo Release 247/247;
headless ASan/UBSan targeted 4/4, portable new test 1/1. ABI 3 / graph 8 unchanged.
Remaining parameter aliases and full standard-type/version conformance keep
stage 1 open. RU/EN: [COUNTED_PROPERTY_KEYS.md](COUNTED_PROPERTY_KEYS.md).

## v0.85 — Canonical formal and registered-symbol identity

Проверено / tested: rightmost named arguments alias for UTF-16-equivalent formal
names, counted NUL formals and named calls, canonical script/debug keys and
registered-class symbols. Regression function_parameter_identity reproduces the
former wrong alias and verifies GC/graph restore; class/native-variable tests
extend symbol/debug coverage. AGG/Cairo Release 248/248 each; restricted headless
ASan/UBSan focused 4/4; portable focused 2/2. Function2 register-only aliases and
remaining type/version conformance remain open. ABI 3 / graph 8 unchanged.
RU/EN: [FUNCTION_PARAMETER_KEYS.md](FUNCTION_PARAMETER_KEYS.md).

## v0.86 — Counted sort fields and selected slice ranges

Проверено: `sortOn` сохраняет полное имя поля, включая NUL, и нормализует
UTF-16 ключи. Заимствованный `slice` допускает источник длиной uint32 max,
если выбранный диапазон не превышает 65536; наследуемые индексы сохраняются.
Ограничение размера результата и остальных полных обходов остаётся.

Tested: complete canonical sortOn field keys and borrowed slice of a small
range from a uint32-length receiver, including inherited slots, negative and
empty ranges. The 65536 selected-range limit and other full-scan limits remain.
Regression: array_counted_ranges. AGG/Cairo Release 249/249 each; focused
headless ASan/UBSan 3/3 (stack/leak exclusions), portable 3/3. Stage 1 remains open.

## v0.87 — Legacy AVM1 coercion

Проверено / tested: Equals/Less numeric coercion and callback order, rooted
operands, conversion failures and general logical operands. avm1_numeric_operations
extended; AGG 249/249, Cairo Release 249/249 (six launch permissions restored and
rerun); focused headless ASan/UBSan and portable 4/4 each. Additional comparison
failure-order assertions passed on AGG. Historical string truth/version coercion
and full stage 2 remain open. RU/EN: [AVM1_NUMERIC_ACTIONS.md](AVM1_NUMERIC_ACTIONS.md).

## v0.88 — Loaded SWF string truth

Проверено / tested: Not/And/Or/If string truth switch for loaded SWF 5/6 vs
7/8, including counted NUL strings. Regression avm1_version_truth; AGG and
Cairo Release 250/250 each after launch-permission reruns (3 and 1 tests);
focused headless ASan/UBSan and portable 3/3 each. Stack/leak sanitizer
exclusions remain. Earlier numeric parsing/function-version rules are open.
RU/EN: [AVM1_NUMERIC_ACTIONS.md](AVM1_NUMERIC_ACTIONS.md).

## v0.89 — AVM1 own-property deletion

Проверено / tested: ActionDelete counted keys, own/prototype/flag behavior,
accessor deletion without getter calls, Array indices/length and coercion errors.
Regression avm1_delete; AGG and Cairo Release 251/251 each (AGG launch-permission
failures and dependent process test rerun); focused restricted headless
ASan/UBSan and portable 3/3 each. Stage 2 remains open.
RU/EN: [AVM1_DELETE.md](AVM1_DELETE.md).

## v0.90 — AVM1 host trace delivery

Проверено / tested: ActionTrace and retained host sink, counted text, conversion
without delivery, callback replacement/errors and GC capture lifetime.
Regression avm1_trace; AGG/Cairo Release 252/252 each after launch-permission
reruns (5 and 1 tests), restricted headless ASan/UBSan and portable 2/2 each.
Historical formatting and reference corpus acceptance remain open.
RU/EN: [AVM1_TRACE.md](AVM1_TRACE.md). ABI 3 / graph 8 unchanged.

## v0.91 — Reproducible top-level SWF trace runner

Реализовано / implemented: opt-in C89 runner for exact trace-byte comparison
with separate failure status. Synthetic FWS/CWS, truncated input and usage
checks passed on Linux AGG. External SWF 6 reference probe fails legacy string
coercion/CallMethod; two selected Array corpus SWFs are version 15 and outside
scope. No new conformance gate is closed.
RU/EN: [SWF_REFERENCE_RUNNER.md](SWF_REFERENCE_RUNNER.md).

## v0.92 — Legacy undefined strings and nullish method calls

Проверено / tested: ordinary undefined ToString/Add2 vs Trace below SWF 7 and
nullish CallMethod result after operand validation. AGG/Cairo Release 253/253
each (AGG two launch-permission reruns); focused restricted headless ASan/UBSan
and portable 4/4 each. Original external SWF 6 undefined_to_string_swf6 finishes
with exact 22-byte reference output. Other numeric/version probes still differ;
stages 1/2 remain open. RU/EN: [SWF_REFERENCE_RUNNER.md](SWF_REFERENCE_RUNNER.md).

## v0.93 — Version-aware AVM1 numbers

Проверено / tested: primitive gates, counted legacy numeric grammar, signed
32-bit radix/exponent wraparound and pre-7 string truth. AGG/Cairo Release
254/254 each; focused restricted headless ASan/UBSan and portable 5/5 each.
Three original external SWF 5/6 traces exactly match pinned reference output.
Historical rounding/object fallback/function provenance and remaining actions
keep stage 2 open. RU/EN: [AVM1_NUMERIC_ACTIONS.md](AVM1_NUMERIC_ACTIONS.md).

## v0.94 — Top-level declarations

Проверено / tested: top-level clip/global DefineLocal/DeclareLocal, complete
coerced names, inherited setters, non-getting declarations and preserved formals.
AGG/Cairo Release 255/255 each (AGG four launch-permission reruns); restricted
headless ASan/UBSan and portable focused 4/4 each. External SWF 8 biturshift
now finishes but reveals a signed-result version difference; that probe still
fails exact comparison. Scope/path conformance remains open.
RU/EN: [AVM1_DECLARATIONS.md](AVM1_DECLARATIONS.md).

## v0.95 — SWF 8 shift result and bitwise coercion order

Проверено / tested: signed SWF 8 URShift result and right-first bitwise/remainder
coercion. AGG/Cairo Release 256/256 each (AGG three launch-permission reruns);
restricted headless ASan/UBSan and portable focused 3/3 each. Fourth original
external SWF trace matches: biturshift_swf8, 93 bytes. Other arithmetic/actions
and whole-stage acceptance remain open. RU/EN: [AVM1_NUMERIC_ACTIONS.md](AVM1_NUMERIC_ACTIONS.md).

## v0.96 — Legacy arithmetic operands

Проверено / tested: legacy arithmetic coercion/order, rooted operands and SWF 4
division-by-zero spelling. AGG/Cairo Release 257/257 each (AGG three
launch-permission reruns); restricted headless ASan/UBSan and portable focused
4/4 each. External SWF 4/8 coercion probes remain blocked by object conversion
or additional missing actions. RU/EN: [AVM1_NUMERIC_ACTIONS.md](AVM1_NUMERIC_ACTIONS.md).

## v0.97 — Numeric valueOf-only object conversion

Проверено / tested: loaded AVM1 numeric valueOf-only lookup, noncallable/missing
methods, object-result primitive gates and recursion recovery. AGG/Cairo Release
258/258 each after launch-permission reruns (3 and 2 tests); restricted headless
ASan/UBSan and portable focused 4/4 each. Four prior exact external SWF matches
reverified. equals_swf4 now completes but unversioned installed NaN/Infinity
globals still differ from its reference. RU/EN: [AVM1_NUMERIC_ACTIONS.md](AVM1_NUMERIC_ACTIONS.md).

## v0.98 — AVM1 string-only object conversion

Проверено / tested: loaded AVM1 toString-only conversion, non-string fallback,
boxed String bypass, counted results and recursion/error recovery. AGG/Cairo
Release 259/259 each (AGG two launch-permission reruns); restricted headless
ASan/UBSan and portable focused 4/4 each. MovieClip paths, Add2 hint behavior
and standard constructor/global version gates remain open.
RU/EN: [AVM1_OBJECT_COERCION.md](AVM1_OBJECT_COERCION.md).

## v0.99 — Standard numeric member names

Проверено / tested: global and standard resource numeric/string member identity,
Object indexed bridge/prototype/accessor/flag behavior and graph transfer. Native
table integer/named separation retained. AGG/Cairo Release 260/260 each after
launch-permission reruns (3 and 1 tests); restricted headless ASan/UBSan and
portable focused 5/5 each. Full standard type/API inventory remains open.
RU/EN: [AVM1_NUMERIC_MEMBERS.md](AVM1_NUMERIC_MEMBERS.md).

## v0.100 — Legacy counted UTF-16 string actions

Проверено / tested: nine legacy string/integer actions, callback order and
converted-string roots across real script-triggered traced GC. AGG/Cairo Release
261/261 each (AGG six launch-permission reruns); restricted headless ASan/UBSan
and portable focused 4/4 each. Non-ASCII pre-SWF-6 length/extract are explicitly
unsupported; default-hint Add2 and remaining actions are open.
RU/EN: [AVM1_LEGACY_STRINGS.md](AVM1_LEGACY_STRINGS.md).

## v0.101 — Primitive coercion roots

Проверено / tested: transient primitive/string roots across script-triggered
traced GC and right-first Add2 conversion. The new regression reproduced the
former failure and now passes thirty repeated coercion cycles. AGG/Cairo
Release 262/262 each (AGG five launch-permission failures and dependent timezone
profile rerun); restricted headless ASan/UBSan and portable focused 4/4 each.
Default-hint/version conformance and whole stages remain open.
RU/EN: [AVM1_OBJECT_COERCION.md](AVM1_OBJECT_COERCION.md).

## v0.102 — AVM1 primitive hint policy

Проверено / tested: valueOf-only ordinary/numeric hints, versioned Date default
hint, preserved default object results and early nonprimitive relational false.
AGG/Cairo Release 263/263 each (AGG two launch-permission reruns); restricted
headless ASan/UBSan and portable focused 5/5 each. Loose equality, player NaN,
MovieClip paths and per-function version provenance remain open.
RU/EN: [AVM1_OBJECT_COERCION.md](AVM1_OBJECT_COERCION.md).

## v0.103 — Unordered relational result type

Проверено / tested: undefined Less2/Greater NaN results, right-first Greater
coercion and corresponding early-false/GC paths. AGG/Cairo Release 264/264 each
(AGG two launch-permission reruns); restricted headless ASan/UBSan and portable
focused 4/4 each. Large external relational SWFs still differ in root/version
behavior; they are not passing acceptance evidence.
RU/EN: [AVM1_NUMERIC_ACTIONS.md](AVM1_NUMERIC_ACTIONS.md).

## v0.104 — SWF 5 loose equality and player NaN behavior

Проверено / tested: SWF 5 preconversion/callback counts, valueOf-only recursive
Equals2, loaded Flash Player 7/8 NaN equality, separate strict/legacy rules and
undefined prototype clearing. AGG/Cairo Release 265/265 each (AGG one
launch-permission rerun); restricted headless ASan/UBSan and portable focused
8/8 each. Three large external equality SWFs finish but still differ in top-level
this/root setup. Prototype attributes, MovieClip path equality and player-profile
selection remain open. RU/EN: [AVM1_OBJECT_COERCION.md](AVM1_OBJECT_COERCION.md).

## v0.105–v0.113 — Rooted reference execution and display target actions

Проверено / tested: stage background frame controls and C89 host color API;
bounded root-timeline runner; four character-code actions; rooted local and
target-prefixed frame Call; host URL request bridge; separate dot/slash clip
paths with legacy callback restoration; counted target resolution; SetTarget/
SetTarget2 activation roots, variable scope and native frame controls; twelve
classic indexed display fields and read-only C89 target introspection.
RU/EN details: [stage](STAGE_BACKGROUND.md), [runner](SWF_REFERENCE_RUNNER.md),
[strings](AVM1_LEGACY_STRINGS.md), [frame Call](AVM1_FRAME_CALL.md),
[URL](AVM1_URL.md), [paths](MOVIECLIP_PATHS.md), [target scope](AVM1_TARGET_SCOPE.md),
[display properties](AVM1_DISPLAY_PROPERTIES.md).

v0.113 evidence: 276/276 core tests in AGG/Cairo, additional AGG runner test,
four focused portable/restricted-sanitizer tests each. Five launch failures
required restored execute bits and successful reruns. Ten original pinned SWF
4–8-scope probes (actual matching assets are SWF 5–8) complete with exact outputs;
large equal/less tests use actual imported roots, without patched movie headers.
Dimensions/system fields, full named-member/rename semantics, historic Unicode
case/codec tables, function source/version provenance, URL loaders/forms and
full SWF resources remain open. All nine roadmap stages remain open; stage 8
still has no implemented native JIT backend.

## v0.114 — native sparse slice / разреженный slice

Removed the 65,536 result-range cap from real/borrowed slice, with ordered live
prototype reads and candidate refresh after getters. Native work and nested
AVM1 callbacks share the instruction limit. Allocation failure tests verify
source preservation and execution-scope restoration. See
[RU/EN details](ARRAY_SPARSE_SLICE.md). Other Array limits and all stages remain open.

Evidence: 278 core tests in both Linux AGG and Cairo Release, plus the AGG
runner; four focused slice/Array tests in restricted ASan/UBSan and portable
profiles. Two AGG launch-permission failures passed after restoring execute
bits. The final added callback-budget and inherited-setter cases passed in
all four profiles. These checks do not certify complete Array or Flash semantics.

## v0.115 — large join / большой join

Removed the real/borrowed join length cap, retaining live inherited reads,
counted separator bytes and recursion cleanup. Native slot/output work shares
the execution limit with callbacks. [RU/EN details](ARRAY_LARGE_JOIN.md).
Evidence: 280 core tests in AGG/Cairo plus the AGG runner, four focused tests
in restricted sanitizer/portable profiles. The allocation test was corrected
from a large fixture whose allocation count exceeded its injection-loop bound
to exhaustive failure injection on a compact fixture; final tests pass in all
four profiles. Two AGG execute-bit failures passed after restoring permissions.
All stages and other Array semantic/size limitations remain open.

## v0.116 — inherited concat / наследуемые индексы concat

Concat now reads inherited indices in array arguments through the shared sparse
copy path, refreshing after getters. Plain borrowed receivers spread by length;
plain arguments stay single values, checked against pinned Ruffle source.
[RU/EN details and remaining historic gaps](ARRAY_CONCAT_SPARSE.md).
Evidence: 282 core tests in AGG/Cairo plus the AGG runner, four focused tests
in restricted sanitizer/portable profiles. Five AGG execute-bit launch failures
passed after permissions were restored. Final cases for later-argument length
changes and uint32 overflow passed in all four profiles. Exhaustive allocation
failures through success retain the source and restore execution nesting.
All roadmap stages remain open; no native JIT or other-OS completion is claimed.

## v0.117 — rejected splice arguments / отклонённые аргументы splice

Missing/undefined starts and explicit undefined/negative deletion counts now
return undefined without splice mutation. Missing starts do not read length.
Zero/null and omitted deletion remain valid; the already-supported undefined
slice endpoint has a regression check. [RU/EN details](ARRAY_UNDEFINED_BOUNDS.md).
Evidence: 283 core tests in AGG/Cairo plus the AGG runner; four focused Array
tests in restricted sanitizer/portable profiles. Three AGG execute-bit launch
failures passed after permission restoration. Final null/omitted/huge-range
cases passed in all four profiles. Initial development compiler errors in
undefined/null value initialization were fixed before running final binaries.
Historic integer/evaluation-order details and all roadmap stages remain open.

## v0.118 — splice coercion order / порядок преобразований splice

Coerce start before length and fast-path selection; avoid a second call to the
original start object when falling back to inherited reads. Removed values are
own result data without inherited setter calls. Errors precede length reads.
[RU/EN details](ARRAY_SPLICE_COERCION.md). Evidence: 284 core tests in AGG/Cairo
plus the AGG runner, four focused Array tests in restricted sanitizer/portable
profiles. One AGG file-writer launch lacked execute permission and its dependent
reader was skipped; both passed after permissions were restored. Delete-count
coercion-driven prototype changes and other historic Array gaps remain open.
All roadmap stages remain open.

## v0.119 — prepared splice bounds / подготовленные границы splice

A shared private preparation captures start, length and deletion once before
choosing the copy path. Delete-count-installed getters now use generic reads
without repeated conversions or length reads. Fast copies exclude late indices
outside the captured source interval. [RU/EN details](ARRAY_SPLICE_BOUNDS.md).
Evidence: 284 core tests in AGG/Cairo plus the AGG runner, four focused Array
tests in restricted sanitizer/portable profiles. Two AGG JPEG tests lacked
execute bits and passed after permission restoration. Getter installation,
source growth/shrinkage and bounds retention are checked. Historic integer,
hole, watcher and result-prototype semantics remain open; all stages are open.

## v0.120 — retained Array result prototype / встроенный прототип результатов

Standard installation publishes a retained original Array prototype as a GC
root. Six native result builders use it independently of custom source or
replacement public prototypes; explicit construction reads the public getter
once. [RU/EN details](ARRAY_RESULT_PROTOTYPE.md). Evidence: 285 core tests in
AGG/Cairo plus the AGG runner, seven focused Array/String tests in restricted
sanitizer/portable profiles (sanitizer still excludes stack/leak certification).
All pass without launch reruns. A real collection after public-prototype
replacement verifies retention and finalization once at teardown. Four newly
fetched original SWF 6/8 probes stop at unsupported FileAttributes/Metadata
tags before execution; no reference success is claimed for them. Imported
uninstalled-context fallback, Array() call semantics and all stages remain open.

## v0.121 — neutral SWF document tags / нейтральные служебные теги

Validate first-position FileAttributes (zero/HasMetadata only) and one declared
scalar UTF-8 Metadata string. Store flags and owned metadata in the clip graph;
new C89 document query leaves output unchanged on errors. Network/AVM2/hardware
policies remain unsupported. [RU/EN details](SWF_DOCUMENT_METADATA.md).
Evidence: 288 core tests in AGG/Cairo plus the AGG runner, four focused tests
in restricted sanitizer/portable profiles. Three AGG bitmap launch-permission
failures passed after restoring execute bits. Final document/API/allocation
tests pass in all four profiles. Ten earlier original SWF probes again complete
with exact outputs. Four new original SWF 6/8 probes now reach execution through
real timelines: Array/Object/InitArray mismatches and unhandled FSCommand remain;
array_trivial's matching Trace followed by failure does not count as acceptance.
Graph transport, real GC, malformed input and allocation failures are covered.
All roadmap stages remain open.

## v0.122 — runner host policy / тестовая политика хоста

Optional quit acknowledgement accepts only the exact counted FSCommand URL
with zero flags and continues bounded execution without external operations.
Default runner behavior is retained; unknown URLs/flags fail. The strict C89
runner build and generated runner test pass. Two original pinned assets now
finish successfully with exact output: add_swf5 (229 bytes, action mode) and
array_trivial SWF 6 (1447 bytes, real root timeline). Twelve probes pass under
explicitly documented modes/policies; actual player-quit behavior is not claimed.
[RU/EN details](SWF_REFERENCE_RUNNER.md). Core code is unchanged from the v0.121
288-test acceptance subset. Array/Object/InitArray gaps and all stages remain open.

## v0.123 — addProperty names/flags / имена и флаги addProperty

Coerce supplied names before descriptor validation, retaining numeric/object
and counted-NUL names. Null setters set read-only; writable replacement clears
that bit. Descriptor allocation failure restores new flag records before stored
values are removed. [RU/EN details](OBJECT_ADD_PROPERTY_NAMES.md). Evidence:
290 core tests in AGG/Cairo plus the AGG runner, four focused Object/Function/
Array checks in restricted sanitizer/portable profiles. Two AGG GC binaries
lacked execute permission and passed after restoration. Exhaustive allocation
retries preserve prior value/flags. Other descriptor/version rules and historic
Array join semantics remain open, as do all roadmap stages.

## v0.124 — loaded-SWF own-data join / исторический join

Loaded join reads own stored elements/length, uses signed wrapping length and
versioned string parts/separators. The existing AVM1 string helper is shared
with standard join/addProperty without changing public C conversion. Unloaded
contexts keep property-aware join. [RU/EN details](ARRAY_LEGACY_JOIN.md).
Evidence: 291 core tests in AGG/Cairo plus the AGG runner; four focused checks
in restricted sanitizer/portable profiles. One AGG GC callback test launch
needed execute-bit restoration and passed. An initial test exposed accidental
use of unversioned host conversion; shared AVM1 conversion fixes the SWF 4..8
spelling checks. All thirteen original probes complete with exact outputs under
documented policies, including array_prototyping SWF8 (121 bytes). Other Array
methods, function-version provenance and every roadmap stage remain open.

## v0.125 — initializer counts / счётчики инициализаторов

Loaded InitArray/InitObject coerce counts, preserve values when signed range
is invalid, truncate valid fractions and map NaN to zero. Valid counts beyond
the remaining materialization cap fail explicitly as unsupported.
[RU/EN details](AVM1_INITIALIZER_COUNTS.md). Evidence: 292 core tests in AGG/Cairo
plus the AGG runner, four focused checks in restricted sanitizer/portable
profiles. Two AGG channel/filter launch failures passed after execute-bit
restoration. The unit test checks preserved stack Trace for both initializers.
Original init_array_invalid remains blocked earlier by required ActionEnd;
no fourteenth oracle success is claimed yet. All roadmap stages remain open.

## v0.126 — bounded AVM1 blocks / ограниченные блоки

SWF frame import and execution accept empty blocks and implicit End exactly at
complete record boundaries; source bytes remain unchanged. Public inspect and
unloaded raw execution stay strict. Branch/invalid payload/graph transfer with
GC checks: [RU/EN details](AVM1_BOUNDED_BLOCKS.md). Evidence: 293 core tests in
AGG/Cairo plus the AGG runner; four focused tests in restricted sanitizer and
portable profiles. Three AGG channel test launches passed after execute-bit
restoration. All fourteen original probes now complete with exact expected
bytes, including init_array_invalid SWF8 (16 bytes, timeline mode). Semantic
version provenance across clip transfer and all roadmap stages remain open.

## v0.127 — initializer GC roots and names / корни и имена

InitObject roots the partial result and temporary names/values through script
coercion and setters; InitArray roots its result through indexed assignment.
Loaded Object keys use versioned AVM1 string conversion.
[RU/EN details](AVM1_INITIALIZER_ROOTS.md). Evidence: 294 core tests in AGG/Cairo
plus the AGG runner and four focused checks in restricted sanitizer/portable
profiles. Four AGG bitmap/filter launch failures passed after restoration and
retries. The final GC test was rebuilt and passed in all four profiles after
strengthening its partially populated object fixture: every script collection
collects at least one object, twenty times. Full literal setter semantics,
missing operands, constructor/version provenance and all stages remain open.

## v0.128 — initializer stack underflow / нехватка операндов

Loaded literals use undefined for missing counts, elements and pair operands;
Array results retain own undefined slots, Object names remain versioned.
Unloaded embedding retains strict errors. [RU/EN details](AVM1_INITIALIZER_UNDERFLOW.md).
Evidence: 295 core tests in AGG/Cairo plus the AGG runner, four focused checks
in restricted sanitizer/portable profiles. Four AGG AVM1 launch failures passed
after execute-bit restoration. Materialization, full constructor/descriptor
semantics, version provenance and all roadmap stages remain open.

## v0.129 — own Array result definitions / собственные индексы

InitArray and String.split bypass inherited accessors/readonly assignment and
define own stored slots. Existing slice/splice/concat share the private helper;
split records final length directly. [RU/EN details](ARRAY_RESULT_OWN_SLOTS.md).
Evidence: 296 core tests in AGG/Cairo plus the AGG runner; five focused checks
in restricted sanitizer/portable profiles. Six AGG Array/font/function launch
failures passed after execute-bit restoration. All fourteen original selected
SWF probes still complete with exact outputs. Full Array descriptors, version
provenance and all roadmap stages remain open.

## v0.130 — plain getter/setter Objects / обычные объекты дескрипторов

addProperty accepts plain Object descriptors; non-function getters yield
undefined, non-function setters ignore writes, and mixed descriptors invoke
the function side. [RU/EN details](OBJECT_DESCRIPTOR_OBJECTS.md). Evidence:
297 core tests in AGG/Cairo plus the AGG runner; five focused checks in
restricted sanitizer/portable profiles. Three AGG Array and two Cairo AVM1
launch failures passed after execute-bit restoration. The descriptor test
includes queue transfer and GC before reading/writing in the receiving context.
Protected replacement, full prototype/Array descriptor rules and all stages
remain open.

## v0.131 — loaded slice own data / исторический slice

Loaded slice captures own data length, uses signed wrapping Int32 endpoints,
ignores inherited/accessor elements and materializes own undefined slots.
Unloaded sparse/property-aware behavior stays available.
[RU/EN details](ARRAY_LEGACY_SLICE.md). Evidence: 298 core tests in AGG/Cairo
plus the AGG runner; five focused checks in restricted sanitizer/portable
profiles. All fourteen selected original SWF probes still complete with exact
output. Original array_slice is SWF15 and is not accepted or patched. Full Array
semantics, version provenance and every roadmap stage remain open.

## v0.132 — loaded concat own data / исторический concat

Loaded concat uses signed own-data length at each source turn, copies own stored
slots and defines explicit undefined results for missing data. Plain Object
arguments remain singletons; ordinary borrowed receivers spread. Unloaded
property-aware sparse behavior remains available.
[RU/EN details](ARRAY_LEGACY_CONCAT.md). Evidence: 299 core tests in AGG/Cairo
plus the AGG runner; five focused checks in restricted sanitizer/portable
profiles. Three AGG bitmap launch failures passed after execute-bit restoration.
Loaded allocation retries preserve source/root state and collect after every
attempt. All fourteen selected original SWFs still complete with exact outputs.
Full Array/version/prototype semantics and every roadmap stage remain open.

## v0.133 — signed splice endpoints / signed-границы splice

Loaded splice converts start/deleteCount through AVM1 signed wrapping Int32
exactly once, preserving conversion order and rejected-count no-mutation.
Unloaded mode retains truncation. [RU/EN details](ARRAY_LEGACY_SPLICE_BOUNDS.md).
Evidence: 300 core tests in AGG/Cairo plus the AGG runner; six focused checks
in restricted sanitizer/portable profiles. Four AGG ActionScript launch failures
passed after execute-bit restoration. Legacy fixtures now exclude their main
when reused, avoiding non-main implicit-return warnings. Splice length remains
property-aware/unsigned; historical removed slots, movement/descriptors and
all roadmap stages remain open.

## v0.134 — own removed splice slots / собственные удалённые элементы

Loaded splice removed results contain own undefined slots for missing/accessor/
inherited data. Generic removed copying bypasses getters; fast copying fills
missing slots, charges the shared budget and retains the partial result graph
with an allocation-free root. [RU/EN details](ARRAY_LEGACY_SPLICE_REMOVED.md).
Evidence: 302 core tests in AGG/Cairo plus the AGG runner; six focused checks
in restricted sanitizer/portable profiles. Five AGG Array/version launch failures
passed after execute-bit restoration. Fast allocation retries preserve source
and root/depth state through success with GC; the initial GC fixture was fixed
to pin its host method Array. All fourteen selected original SWFs still complete
with exact outputs. Historical source movement/write and signed own-data length,
generic mutation atomicity and all roadmap stages remain open.

## v0.135 — splice own signed length / собственная signed-длина

Loaded splice ignores virtual/inherited length, captures own data with Int32
wrapping after start and before deleteCount, and explicitly rejects negative
signed mutation lengths before deletion coercion/mutation. Unloaded mode stays
property-aware/unsigned. [RU/EN details](ARRAY_LEGACY_SPLICE_LENGTH.md).
Evidence: 303 core tests in AGG/Cairo plus the AGG runner; six focused checks
in restricted sanitizer/portable profiles. Five AGG Array/bitmap and three Cairo
bitmap launch failures passed after execute-bit restoration. Negative signed
mutation indices remain unsupported; this early rejection is not claimed as
full historical behavior. Source set_data/movement and all stages remain open.

## v0.136 — retained accessor data / data под getter

addProperty retains prior own stored values separately from getter/setter and
preserves them on descriptor replacement. Loaded Array reads/length use this
data; plain getters fall back to it. Existing hidden descriptor roots preserve
strings and cycles through graph transport without a wire-structure change.
[RU/EN details](OBJECT_ACCESSOR_DATA.md). Evidence: 304 core tests in AGG/Cairo
plus the AGG runner; seven focused checks in restricted sanitizer/portable
profiles. Traced NUL strings, owner cycles and virtual length survive GC and
queue transfer. All fourteen original selected SWFs still match exactly.
Ordinary assignment latent-data updates, replacement flags, native accessor/
method data and full version semantics remain open, as do all roadmap stages.

## v0.137 — accessor assignment data / data при присваивании

Successful ordinary setters update retained data, including non-function
setter Objects; callable getters still execute and plain getters return stored
data. Counted strings/cycles survive GC and transfer; readonly/error cases
preserve data. An allocation-free descriptor root spans setter execution.
[RU/EN details](OBJECT_ACCESSOR_ASSIGNMENT.md). Evidence: 305 core tests in
AGG/Cairo plus the AGG runner; six focused checks in restricted sanitizer/
portable profiles. Four AGG bitmap and three Cairo Function/inheritance launch
failures passed after execute-bit restoration. All fourteen selected original
SWFs still match exactly. Array mutation set_data, replacement flags, inherited
setter data, thrown-setter/version rules and all stages remain open.

## v0.138 — InitObject empty names / пустые имена

Loaded InitObject coerces names, then ignores an empty result; unloaded host
mode retains empty keys. Earlier underflow/GC expectations for SWF6 were
incorrect and are corrected. [RU/EN details](AVM1_INITIALIZER_EMPTY_NAMES.md).
Evidence: 306 core tests in AGG/Cairo plus the AGG runner; five focused checks
in restricted sanitizer/portable profiles. Eight AGG Array/bitmap and four
Cairo Array launch failures passed after execute-bit restoration. Fifteen
original SWFs now complete with exact expected output, including newly selected
init_object_invalid SWF8 (16 bytes). add_property/init_object_order are SWF15
and do not count as supported successes. General empty-name property behavior
and all roadmap stages remain open.

## v0.139 — preserved virtual flags / flags замены

addProperty preserves existing own-property flags, allows DONT_DELETE descriptor
replacement while keeping deletion protection, and marks only new null-setter
properties readonly. Writable existing null setters use the script data bridge.
[RU/EN details](OBJECT_ADD_PROPERTY_FLAGS.md). Evidence: 307 core tests in
AGG/Cairo plus the AGG runner; six focused checks in restricted sanitizer/
portable profiles. One AGG bitmap launch failed and passed after execute-bit
restoration. Full suites used serial CTest scheduling for this change.
All fifteen selected original SWFs still match exactly. Corrected names and
allocation fixtures now retain existing flags. Real Array index/length guards,
readonly callbacks, inherited setter data and all roadmap stages remain open.

## v0.140 — inherited setter ownership / владелец setter

CAPI setter dispatch scopes actual lookup ownership with allocation-free roots
for owner, receiver and capture. Inherited script setters execute with receiver
without changing prototype data; nested own setters restore prior metadata.
Existing descriptors need no new wire fields.
[RU/EN details](OBJECT_INHERITED_SETTER_DATA.md). Evidence: 308 core tests in
AGG/Cairo plus the AGG runner; seven focused checks in restricted sanitizer/
portable profiles. Two AGG property/prototype launch failures passed after
execute-bit restoration. Nested setters, counted NUL, GC/queue transfer and
owner restoration after errors pass; all fifteen original SWFs still match.
Readonly callback rules, self-detachment/replacement, Array mutation set_data,
versions and every roadmap stage remain open.

## v0.141 — loaded readonly setter execution / readonly callback

Loaded script setters execute on readonly descriptors while current own flags
control retained data after callback. Clearing readonly inside the setter enables
storage; inherited readonly data permits own shadowing. Unloaded C checks remain
strict. A scoped owned counted key survives callback mutation/nesting; copy
failure precedes metadata changes. [RU/EN details](OBJECT_READONLY_SETTER.md).
Evidence: 309 core tests in AGG/Cairo plus the AGG runner; seven focused checks
in restricted sanitizer/portable profiles. Three AGG GC launch failures passed
after execute-bit restoration. Numeric/countеd NUL callbacks and metadata restore
pass; all fifteen original SWFs still match. Readonly watcher order, self-replaced
property storage, real Array virtual guards/version rules and all stages stay open.

## v0.142 — setter replacement / замена setter

Successful own script setters store into the current receiver property after
callback deletion/replacement, without another setter invocation; current own
readonly suppresses storage. Counted bytes and both descriptor graphs remain
owned/rooted. [RU/EN details](OBJECT_SETTER_REPLACEMENT.md). Evidence: 310 core
tests in AGG/Cairo plus the AGG runner; seven focused tests in restricted
heap/global ASan+UBSan and portable Linux profiles. One AGG blur-filter launch
failure passed after execute-bit restoration. Fifteen original SWFs still match
exactly. Native C accessor replacement remains UNSUPPORTED; callback side effects
are not rolled back. Array mutation set_data and all stages stay open.

## v0.143 — loaded reverse own data / reverse собственных данных

Loaded reverse reads own signed length and raw element data, ignores prototypes
and accessors, deletes paired positions before storing without setter callbacks,
and materializes missing undefined values. Protected virtual data retains own
flags. [RU/EN details](ARRAY_LEGACY_REVERSE.md). Evidence: 311 core tests in
AGG/Cairo plus the AGG runner; five focused checks in restricted heap/global
ASan+UBSan and portable Linux profiles. One AGG bitmap-allocation launch failure
passed after execute-bit restoration. All fifteen original SWFs still match
exactly. The mutation size limit/native accessor model, other mutators and every
roadmap stage remain open.

## v0.144 — loaded shift/pop raw data / удаление элементов

Loaded shift/pop use own raw data without getter/prototype lookup. Borrowed
length remains unchanged; only native Arrays shrink. Protected native tail
indices may remain beyond the smaller length instead of clamping it.
[RU/EN details](ARRAY_LEGACY_REMOVE.md). Evidence: 312 core tests in AGG/Cairo
plus the AGG runner; six focused checks in restricted heap/global ASan+UBSan
and portable Linux profiles. One AGG broker-lifetime launch failure passed after
execute-bit restoration. All fifteen original SWFs match exactly. Negative signed
lengths, shift size limits, native accessor data and all stages remain open.

## v0.145 — empty Object assignment / пустое имя свойства

Loaded ordinary Object writes ignore empty keys before property callbacks,
while SetMember still coerces the key once. Preexisting empty host properties
remain readable; single counted NUL keys remain writable.
[RU/EN details](OBJECT_EMPTY_ASSIGNMENT.md). Evidence: 313 core tests in
AGG/Cairo plus the AGG runner; five focused checks in restricted heap/global
ASan+UBSan and portable Linux profiles. No launch failures in these suites.
All fifteen original SWFs still match exactly. SetVariable/local scope, function
versions and every roadmap stage remain open.

## v0.146 — ASSetPropFlags set precedence / приоритет установки

The AS builtin clears before setting overlapping flags; its set bits win.
The direct C flag extension remains unchanged.
[RU/EN details](STANDARD_PROPERTY_FLAGS.md). Evidence: 314 core tests in
AGG/Cairo plus the AGG runner; four focused checks in restricted heap/global
ASan+UBSan and portable Linux profiles. Exhaustive 512 flag combinations pass
in unloaded mode and each SWF4..8; counted NUL, readonly and own selection pass.
Four AGG and two Cairo launch failures plus an AGG runner PermissionError passed
after execute-bit restoration. Fifteen original SWFs still match exactly.
Selector/default coercions, version masks and all stages remain open.

## v0.147 — ASSetPropFlags selectors/defaults / выбор свойств

Only NULL selects all own fields. Undefined is converted with loaded AVM1
string rules (empty in SWF4..6, "undefined" in SWF7..8); missing masks are zero,
and missing object/list returns undefined without mutations.
[RU/EN details](STANDARD_PROPERTY_FLAGS.md). Evidence: 314 core tests in
AGG/Cairo plus the AGG runner; four focused checks in restricted heap/global
ASan+UBSan and portable Linux profiles. No launch failures. Existing C89 tests
cover each selector/default path and all 512 mask combinations in each mode.
Fifteen original SWFs still match. Full receiver/mask conversion and all stages
remain open.

## v0.148 — loaded watcher names/data / watcher в SWF

Loaded watch/unwatch coerce names before callback validation, allow empty and
virtual names, and read old values through stored own/prototype data without
getters/metatable hooks. Native field/data restrictions remain explicit.
[RU/EN details](OBJECT_LEGACY_WATCH.md). Evidence: 315 core tests in AGG/Cairo
plus the AGG runner; five focused checks in restricted heap/global ASan+UBSan
and portable Linux profiles. Three AGG bitmap launch failures passed after
execute-bit restoration. Numeric/object/empty/counted NUL, GC and inherited
setter ownership pass; fifteen original SWFs still match. Readonly and exception
ordering, native accessor data, case/version rules and all stages stay open.

## v0.149 — watcher before readonly / watcher до readonly

Loaded ordinary/indexed writes run watchers before current readonly checks.
Callbacks can alter flags, indexed setter dispatch consumes prepared data once,
and lookup ownership is rooted through detachment.
[RU/EN details](OBJECT_READONLY_WATCH.md). Evidence: 316 core tests in
AGG/Cairo plus the AGG runner; six focused checks in restricted heap/global
ASan+UBSan and portable Linux profiles. Three AGG Array launch failures passed
after execute-bit restoration. Named/numeric/countеd NUL, virtual setter flags,
root unwind and post-write GC pass; all fifteen original SWFs still match.
Native Array guard and watcher exception ordering, case/version and all stages
remain open.

## v0.150 — registerClass name validation / имя класса

Constructor validation precedes name conversion; loaded registration uses the
AVM1 string bridge and permits empty symbols. Null/undefined removes entries.
[RU/EN details](OBJECT_REGISTER_CLASS_NAMES.md). Evidence: 317 core tests in
AGG/Cairo plus the AGG runner; seven focused checks in restricted heap/global
ASan+UBSan and portable Linux profiles. Invalid constructors skip name callbacks;
empty/undefined/counted NUL, GC and queue transfer with restored execution pass.
Four AGG launch failures, one dependent reader and one subprocess PermissionError
passed on explicit rerun after restoring executable bits. Fifteen original SWFs
still match; case-sensitive environment separation, function versions and all
stages remain open.

## v0.151 — single property-name coercion / однократное имя

Own/enumeration queries coerce a counted key once with loaded AVM1 rules, and
return false without an argument. Enumeration presence and flags share that
key instead of invoking toString twice.
[RU/EN details](OBJECT_PROPERTY_QUERIES.md). Evidence: 318 core tests in
AGG/Cairo plus the AGG runner; four focused checks in restricted heap/global
ASan+UBSan and portable Linux profiles. Changing callback names, missing versus
undefined, hidden flags and counted NUL pass across unloaded/SWF4..8. Two AGG
bitmap launch failures passed after execute-bit restoration. Fifteen original
SWFs still match. Case/version/native fields and all stages remain open.

## v0.152 — loaded unshift raw data / unshift собственных данных

Loaded unshift moves raw own data without accessor/watcher callbacks, materializes
holes even with no arguments and preserves borrowed length. Native readonly
length allows out-of-length raw slots. Length coercion shares traversal work
accounting in loaded unshift/shift/pop.
[RU/EN details](ARRAY_LEGACY_UNSHIFT.md). Evidence: 319 core tests in
AGG/Cairo plus the AGG runner; six focused checks in restricted heap/global
ASan+UBSan and portable Linux profiles. Protected virtual data, counted NUL/GC,
native/borrowed lengths and combined valueOf+traversal budget pass. Three AGG
bitmap/shape/mutex launch failures passed after execute-bit restoration. Fifteen
original SWFs still match. Signed/size limits, native accessor data and all
roadmap stages remain open; partial mutation is not rolled back.

## v0.153 — loaded push raw data / push собственных данных

Loaded push writes raw own data without descriptor deletion or accessor/watch
callbacks and updates raw length under own flags. Native readonly length permits
raw slots beyond it. [RU/EN details](ARRAY_LEGACY_PUSH.md). Evidence: 320 core
tests in AGG/Cairo plus the AGG runner; six focused checks in restricted
heap/global ASan+UBSan and portable Linux profiles. Virtual length/index, readonly
data, no watchers and counted NUL/GC pass. One AGG error-transfer launch failure
and its dependent subprocess PermissionError passed after execute-bit restoration.
Fifteen original SWFs still match. Signed/native accessor limits, case/version
and all stages remain open; no rollback on partial mutation is promised.

## v0.154 — generic splice raw moves / общий перенос splice

Loaded generic splice moves by own presence/raw data, bypasses callbacks/watch,
handles both directions and retains borrowed tails beyond new length. Native
generic protected tails do not clamp length. Getter-only C accessor raw writes
retain their immutable contract; writable native accessor data is unsupported.
[RU/EN details](ARRAY_LEGACY_SPLICE_MUTATION.md). Evidence: 321 core tests in
AGG/Cairo plus the AGG runner; nine focused checks in restricted heap/global
ASan+UBSan and portable Linux profiles. An initial getter-only length regression
was corrected; all repeated suites pass without launch failures. Virtual flags,
prototype holes, borrowed/native tails and counted strings after traced GC pass.
Fifteen original SWFs still match. Native fast splice watch/flags, signed/size
limits, native accessor data and all stages remain open.

## v0.155 — native splice flags/watch / native флаги splice

Loaded indexed flags/readonly length route native splice through live generic
raw operations, respecting source deletion before later destination writes.
The ordinary unflagged fast path skips watchers while retaining staging/OOM
checks. [RU/EN details](ARRAY_LEGACY_SPLICE_FLAGS.md). Evidence: 322 core tests
in AGG/Cairo plus the AGG runner; nine focused checks in restricted heap/global
ASan+UBSan and portable Linux profiles. All eight indexed masks, protected tails,
readonly length, watcher bypass and inherited getter holes pass. The final pinned
prototype fixture passed all four profiles. Two AGG bitmap/broker launch failures
passed after execute-bit restoration. Fifteen original SWFs still match. Routed
flagged array size limits, sort mutation/case/version and all stages remain open.

## v0.156 — native sort primitive keys / примитивные ключи sort

Loaded native fast sort/sortOn skips commit watchers and converts primitive/field
keys with the AVM1 string bridge. SWF4 Boolean and pre-SWF7 missing fields use
versioned strings. [RU/EN details](ARRAY_LEGACY_SORT_KEYS.md). Evidence: 323 core
tests in AGG/Cairo plus the AGG runner; four focused checks in restricted
heap/global ASan+UBSan and portable Linux profiles. Unloaded/SWF4..8 watcher,
Boolean and missing sortOn field ordering pass. Four AGG gradient/table/codec
launch failures passed after execute-bit restoration. All fifteen original SWFs
still match. Generic property/prototype reads/writes, callback order, flags/holes
and all roadmap stages remain open.

## v0.157 — generic sort own snapshot / собственный снимок sort

Loaded generic sort/sortOn snapshots own raw element data and signed length,
materializes undefined holes and writes raw data without index getters/setters/
watchers. Snapshot has an explicit GC root through callbacks/copyback.
[RU/EN details](ARRAY_LEGACY_SORT_DATA.md). Evidence: 324 core tests in
AGG/Cairo plus the AGG runner; five focused checks in restricted heap/global
ASan+UBSan and portable Linux profiles. Borrowed virtual length/index, inherited
holes, readonly data, no index callbacks and post-GC data pass. Five AGG Array/GC
launch failures passed after execute-bit restoration. Fifteen original SWFs
still match. Native sparse holes, callback/flag/return rules, native accessor data
and all stages remain open.

## v0.158–v0.160 — loaded sorting corrections / исправления сортировки

Loaded Array.sort handles DESCENDING after comparison by reversing the complete
result, including equal keys. Undefined uses versioned AVM1 string keys and is
passed to custom comparators; the SWF6 v0.157 expectation is corrected. Loaded
sortOn returns the original receiver, including generic borrowed copyback.
[Direction/return details](ARRAY_LEGACY_SORT_COMPARATOR.md) and
[raw-data/undefined details](ARRAY_LEGACY_SORT_DATA.md) describe RU/EN behavior.
All earlier stage completion states remain open. Exact comparator order,
custom UNIQUESORT, native sparse holes and sortOn field lookup remain open.

Загруженный sort разворачивает весь результат для DESCENDING, включая равные
ключи. undefined сравнивается с учётом версии AVM1 и передаётся comparator.
sortOn возвращает исходный receiver, включая заимствованный метод.
Исправлено ожидание SWF6 из v0.157. Все стадии остаются открытыми.

Evidence through v0.160: AGG 326/Cairo 325 tests, five focused checks in
portable Linux and restricted ASan/UBSan, and fifteen byte-exact original SWFs.
AGG execute-bit launch failures passed after restoring executable bits.
Полные проверки AGG/Cairo и профильные portable/ASan/UBSan прошли;
15 исходных SWF совпали побайтово. Это не подтверждает полную совместимость.

## v0.161 — splice source retention / сохранение source index

Corrected the v0.154/v0.155 source-deletion interpretation: pinned Ruffle
2372a4e splice_internal copies values without deleting sources. Generic loaded
splice now retains source properties/flags; native length shrinking separately
removes trailing indices. Borrowed tails remain beyond the new length. All eight
indexed masks now retain their flags, and readonly blocks subsequent writes.
[RU/EN details](ARRAY_LEGACY_SPLICE_FLAGS.md). Comparator order, large/signed
Array limits and all roadmap stages remain open.

Исправлено прежнее ошибочное удаление источника: splice копирует данные,
сохраняя флаги. Только native length удаляет хвост; borrowed хвост остаётся.
Это исправляет описание и ожидания v0.154/v0.155. Все стадии открыты.

Evidence v0.161: 326 AGG and 325 Cairo tests, eight focused checks each in
portable Linux and restricted heap/global ASan+UBSan, fifteen byte-exact original
SWFs. Cairo timezone launch failure and its dependent profile test passed after
execute-bit restoration. Проверки пройдены; stage completion не заявлено.

## v0.162 — rooted loaded sort snapshot / полный снимок сортировки

Loaded sort captures all own values before key coercion; native sparse holes
become own undefined values. An explicit FVM_Value collector frame retains the
complete original graph through callbacks and restores caller roots on all exits.
[RU/EN details](ARRAY_LEGACY_SORT_SNAPSHOT.md). Evidence: AGG 327/Cairo 326,
six focused portable/restricted heap-global ASan+UBSan tests, fifteen byte-exact
original SWFs. Seven AGG launch failures passed after execute-bit restoration.
Tests verify coercion clearing the receiver, original rows after actual post-sort
GC, callback error root/depth restoration and native undefined holes. Arbitrary
native callback GC remains limited by the existing native barrier. Callback order,
custom UNIQUESORT and all roadmap stages remain open.

Полный снимок захватывается до toString и сохраняется в корнях GC. Проверены
очистка receiver из callback, сохранение исходных объектов и sparse native holes.
Сборка выполняется после sort; GC внутри произвольного native callback не заявлен.

## v0.163 — loaded quicksort / порядок сравнений

Loaded sort/sortOn uses left-pivot quicksort with the reference LIFO comparison
order; unloaded stable merge remains unchanged. Every comparison/partition shares
the instruction budget and cancellation checks. [RU/EN details](ARRAY_LEGACY_SORT_ORDER.md).
Evidence: AGG 327/Cairo 326, six focused portable/restricted heap-global ASan+UBSan
checks, fifteen byte-exact original SWFs. Three AGG launch failures passed after
execute-bit restoration. Seven pinned comparator pairs, DESCENDING/indexed return,
third-comparison error, root/depth restoration, pre-callback budget exhaustion and
sorted worst-case bounds pass. Eager builtin/field keys, custom UNIQUESORT and all
stages remain open.

Проверены эталонные пары comparator, ошибки и предел работы quicksort.
Точная семантика преобразования ключей и все стадии пока открыты.

## v0.164 — comparison-time coercion / преобразования пар

Loaded Array.sort performs builtin AVM1 string conversion per comparison, left
before right, while NUMERIC compares two Numbers directly. Intermediate operand/
string roots remain live across later callbacks. Custom UNIQUESORT checks adjacent
final values with builtin comparison instead of invoking the custom callback.
[RU/EN details](ARRAY_LEGACY_SORT_COERCION.md). Evidence: AGG 328/Cairo 327,
seven focused portable/restricted heap-global ASan+UBSan checks, fifteen byte-exact
original SWFs. Fourteen pinned toString calls, conversion errors, equal/positive
custom uniqueness and snapshot clearing pass. A test-local expired string buffer
was replaced by a literal; all seven final focused tests passed all four profiles.
One AGG bitmap launch failure passed after restoring executable bits. sortOn field
lookup/coercion, native GC barriers and all stages remain open.

Проверены повторные преобразования каждой пары и отдельная builtin проверка
UNIQUESORT. Исправлена ошибка времени жизни строки в тесте. Все стадии открыты.

## v0.165 — own live sortOn fields / собственные данные полей

Loaded sortOn reads own stored fields at each comparison without getters/prototype
lookup, using retained script descriptor data, native Array length/index values and
lazy native method values. Field coercion is left before right; changes from
callbacks are visible later. Primitive elements use full first-field options.
[RU/EN details](ARRAY_LEGACY_SORT_FIELDS.md). Evidence: AGG 328/Cairo 327,
seven focused portable/restricted heap-global ASan+UBSan checks, fifteen byte-exact
original SWFs. Counted NUL fields, pinned ascending/descending conversions,
callback field changes, inherited getter bypass, versioned missing values,
conversion errors and primitive numeric descending pass. Two AGG launch failures
passed after execute-bit restoration. Native display fallback, SWF case/version
masks, argument overloads and all stages remain open.

Проверены собственные virtual данные, повторное чтение изменённых полей и
отсутствие getter. Native display fallback и все стадии остаются открытыми.

## v0.166 — loaded raw sort copyback / обратная запись sort

Loaded native sort/sortOn writes captured positions sequentially through shared
raw element storage, preserving current receiver length/tails and readonly length
semantics after callback changes. Unloaded staging is unchanged.
[RU/EN details](ARRAY_LEGACY_SORT_COPYBACK.md). Evidence: AGG 328/Cairo 327,
seven focused portable/restricted heap-global ASan+UBSan checks, fifteen byte-exact
original SWFs. Callback tail growth, readonly length one/zero with own outside
slots, errors after growth, indexed return with captured length, restored roots/
depth and getter/watcher bypass pass. Three AGG launch failures passed after
restoring execute bits. Sequential loaded copyback may leave partial mutation on
allocation/budget errors; full argument/flag cases and all stages remain open.

Проверены сохранение callback хвоста, READ_ONLY length и индексы за длиной.
Ошибки обратной записи могут оставить частичные изменения. Все стадии открыты.
