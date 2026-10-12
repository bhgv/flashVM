# Оставшаяся работа после v0.58 / Remaining work after v0.58

Основной план: ROADMAP.md. Ни один этап ещё не закрыт. Уже проверенные части
не нужно реализовывать заново; ниже перечислена оставшаяся работа и приёмка.

1. **Типы и базовые объекты:** полная сверка UTF-16/Unicode, Object/Array,
   ключей, длины, заимствованных методов, наследования и registerClass с Flash.
2. **AVM1:** точная семантика областей видимости, регистровых aliases,
   arguments/super, исключений, преобразований и различий версий SWF;
   оставшиеся Action-коды и реальный корпус SWF.
3. **MovieClip/SWF:** полный display list и ресурсные теги, маски/clipDepth,
   размеры и shape hit testing, drag/input, загрузка клипов, текстовые поля,
   удаление/повторное создание и ссылки по путям; полная сверка timeline.
4. **Графика, текст и медиа:**
   - BitmapData: оставшиеся операции, draw, точные правила premultiplied alpha, загрузка ресурсов,
     полная семантика attachBitmap и перенос состояния; JPEG/lossless/PNG/GIF теги
     подключены в проверенном subset v0.45–v0.48, остаётся эталонная сверка;
   - оставшиеся режимы градиентов и Shape4/LineStyle2, маски, blend modes,
     cacheAsBitmap, scrollRect и все фильтры Flash 8;
   - полные TextField/TextFormat, текстовые SWF-теги, встроенные/системные
     шрифты, Unicode-layout, переносы, bidi/shaping, редактирование и выделение;
   - Sound, SWF audio/streaming, поддерживаемые кодеки, синхронизация и устройства;
   - видео SWF/FLV до Flash 8, соответствующие API/кодеки, декодирование и тайминг;
   - оставшаяся точная семантика Point/Matrix/Rectangle/ColorTransform/Transform,
     complete pixelBounds, getter/coercion/Math overrides и fixed-point правила;
   - приёмка реальными SWF, эталонным поведением, пиксельными/текстовыми/медийными
     проверками, GC/OOM, queue/file/network и документацией RU/EN.
5. **Другие API:** полный каталог и недостающие загрузчики, XML, сетевые объекты,
   SharedObject, события, ввод, системные службы и стандартные объекты AS1/AS2.
6. **Расширения:** полная семантика потоков/каналов/мутексов/метатаблиц,
   ресурсная идентичность, все виды транспорта и обрывов, распределённые
   блокировки, Lua-подобный C API и длительные конкурентные испытания.
7. **GC/отладчик:** полный аудит новых корней, циклов, финализации и конкуренции;
   source locations, дедлоки, перехват/заморозка каналов и анализ посылок.
8. **JIT:** ARM/ARM64/x86/x64/RISC-V/RISC-V64, согласование с GC/исключениями/
   отладчиком и проверка эквивалентности интерпретатору.
9. **Платформы/выпуск:** реальные сборки и запуски Windows/Linux/macOS/Android,
   адаптеры вывода, статические зависимости, воспроизводимость и полная приёмка.

Проверенные новые части: Number power-of-two formatting (v0.39), числовые/битовые
Action-коды (v0.40), векторный shape hitTest (v0.41), copyPixels (v0.42), beginBitmapFill и sampler (v0.43).
Следующая задача пункта 4: оставшиеся BitmapData операции и filters; полная scope семантика loadBitmap ещё требует сверки. Версия и число тестов
не означают завершения пункта. Технические результаты хранятся отдельно от Git.

## English

All nine roadmap stages remain open. Remaining work comprises primitive/Object/
Array conformance; complete AVM1/version semantics; full MovieClip/display-list
resources and interaction; bitmap graphics, remaining gradients/strokes/masks/
filters/blending, complete text/fonts/layout/editing, audio/video/codecs/timing;
remaining standard APIs; complete concurrency/transport/metatables/C extensions;
GC/debugger acceptance; all six JIT targets; actual four-platform release runs.
Stage 4 also requires completing geometry conformance and real SWF/reference,
rendering/media, GC/allocation-failure and queue/file/network acceptance tests.
The next concrete task is BitmapData and bitmap fills. No completion percentage,
deadline or readiness is implied by the current version or test counts.

В v0.45 проверены DefineBitsLossless/2 и bitmap-заливки Shape1/2/3; JPEG-теги и loadBitmap/draw остаются открытыми (SWF_LOSSLESS_BITMAPS.md).

v0.45 checks lossless SWF resources and Shape1/2/3 bitmap fills; JPEG tags and loadBitmap/draw remain open.

В v0.46 подключены JPEG2/3 через bounded C89/libjpeg адаптер; legacy JPEG tables, embedded PNG/GIF и CMYK открыты (SWF_JPEG_BITMAPS.md).

v0.46 imports JPEG2/3 through a bounded C89 libjpeg bridge; legacy tables, embedded PNG/GIF and CMYK remain open.

В v0.47 проверены embedded PNG и full-canvas single-frame GIF89a в SWF 8 (SWF_PNG_GIF_BITMAPS.md). Legacy JPEG tables и полная alpha/color сверка остаются открытыми.

v0.47 checks SWF 8 PNG and full-canvas single-frame GIF89a resources; legacy JPEG tables and exact color/alpha conformance remain open.

В v0.48 проверены DefineBits/JPEGTables и размещение bitmap в SWF 1..8 (SWF_JPEG_TABLES.md); несовпадения/recovery и точная alpha/color семантика открыты.

v0.48 checks shared JPEG tables and bitmap placement across SWF 1..8; exact recovery/color conformance remains open.

В v0.49 проверен static loadBitmap с независимыми копиями и AVM1/global resource scopes, GC/OOM и queue/file/TCP (BITMAP_LOAD_RESOURCE.md).

v0.49 checks static loadBitmap copies, current/global resource scope, GC/OOM and queue/file/TCP; full reference scope conformance remains open.

В v0.50 проверен draw BitmapData/MovieClip, transforms/clipping, 12 blend modes, self-alias, GC/OOM и queue (BITMAP_DRAW.md). Alpha/erase и точная group/Flash семантика остаются открытыми.

v0.50 checks draw with 12 blend modes; alpha/erase coverage and exact grouping/reference conformance remain open.

В v0.51 добавлено geometry coverage для alpha/erase draw, AA/holes/strokes и single getter read. Все 14 draw IDs реализованы в описанной модели; полная Flash/group/filter сверка остаётся открытой.

v0.51 adds alpha/erase geometry coverage and one-pass getter evaluation; complete Flash/group/filter conformance remains open.

В v0.52 проверен backend-independent scanline floodFill, scratch budget, reentrant disposal, GC/queue и allocation rollback/balance (BITMAP_FLOOD_FILL.md).

v0.52 checks transactional bounded floodFill; exact coercion/alpha conformance and remaining stage 4 APIs remain open.

В v0.53 проверен getColorBoundsRect с typed captured Rectangle, masked matching, GC/queue и OOM/balance (BITMAP_COLOR_BOUNDS.md).

v0.53 checks masked color bounds and captured Rectangle factories; exact Flash conformance and remaining APIs remain open.

В v0.54 проверены unsigned threshold, copySource, shared aligned clipping/snapshot, aliases, GC/queue и OOM/balance (BITMAP_THRESHOLD.md).

v0.54 checks masked threshold operations and common region snapshots; exact Flash conformance and remaining APIs remain open.

В v0.55 проверен per-channel merge с aligned snapshot, weights/rounding, aliases, GC/queue и OOM/balance (BITMAP_MERGE.md).

v0.55 checks channel merging; exact premultiplied/coercion conformance and remaining APIs remain open.

В v0.56 проверен paletteMap: cached palettes, modulo-32 sums, source aliases, GC и OOM rollback (BITMAP_PALETTE_MAP.md).
v0.56 checks palette mapping; exact historical Flash coercion and premultiplied storage remain open.

В v0.57 проверен hitTest: point/rectangle/bitmap, alpha thresholds, integer bounds, queue/file, GC и OOM (BITMAP_HIT_TEST.md).
v0.57 checks pixel hit testing; reference coercion/accessor compatibility remains open.

В v0.58 подключены BitmapFilter/ColorMatrixFilter, matrix snapshots/clone и BitmapData.applyFilter/generateFilterRect для color matrix; остальные фильтры и MovieClip.filters открыты (COLOR_MATRIX_FILTER.md).
v0.58 adds the first filter classes and color matrix bitmap application; remaining filters and scene filter rendering stay open.
