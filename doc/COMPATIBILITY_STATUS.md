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
