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
