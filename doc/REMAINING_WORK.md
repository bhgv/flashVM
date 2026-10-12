# Оставшаяся работа после v0.47 / Remaining work after v0.47

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
     полная семантика attachBitmap, bitmap-теги SWF и перенос состояния;
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
Следующая задача пункта 4: SWF bitmap-ресурсы, draw и оставшиеся BitmapData операции. Версия и число тестов
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
