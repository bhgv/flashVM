# BitmapData: текущая реализация / Current implementation

## Current development checkpoint: v0.64 / Текущая контрольная точка

Ниже сохранены исторические проверки v0.37–v0.42. Более поздние реализации
описаны отдельно: [bitmap resources and JPEG tables](SWF_JPEG_TABLES.md),
[MovieClip bitmap fills](MOVIECLIP_BITMAP_FILL.md),
[attachBitmap](MOVIECLIP_ATTACH_BITMAP.md), [loadBitmap](BITMAP_LOAD_RESOURCE.md),
[draw/blend coverage](BITMAP_DRAW.md), [floodFill](BITMAP_FLOOD_FILL.md),
[color bounds](BITMAP_COLOR_BOUNDS.md), [threshold](BITMAP_THRESHOLD.md),
[merge](BITMAP_MERGE.md), [paletteMap](BITMAP_PALETTE_MAP.md),
[hitTest](BITMAP_HIT_TEST.md) и [ColorMatrixFilter](COLOR_MATRIX_FILTER.md) [ConvolutionFilter](CONVOLUTION_FILTER.md) и [noise](BITMAP_NOISE.md) и [perlinNoise](BITMAP_PERLIN.md).

Остаются эталонная сверка pixelDissolve (модель v0.64 описана в BITMAP_DISSOLVE.md), остальные applyFilter/generateFilterRect
варианты, полная premultiplied/coercion сверка и scene filter rendering.
Наличие метода и успешные сценарии не означают полную Flash-совместимость.

Historical v0.37–v0.42 evidence is retained below. The linked later documents
cover resource integration, bitmap scene rendering and additional pixel methods.
Pixel dissolve, remaining filter variants, exact historical
premultiplied/coercion conformance and scene filters remain open.

Реализован класс `flash.display.BitmapData`: ограниченные размеры 1–2880,
прозрачные/непрозрачные буферы, width/height/transparent/rectangle только для
чтения, clone, dispose, getPixel, getPixel32, setPixel, setPixel32, fillRect,
scroll, colorTransform и copyChannel. getPixel32 возвращает знаковое Int32; значения цветов
при записи преобразуются в Uint32. setPixel сохраняет альфа-канал. scroll
сохраняет края вне области переноса; перекрывающиеся пиксели читаются из снимка.
colorTransform применяет восемь нативных каналов к пересечению прямоугольника
с изображением, ограничивает результат диапазоном 0–255; непрозрачность
непрозрачного bitmap сохраняется. dispose освобождает пиксели, clone независим.

Состояние передаётся через общий графовый codec, очередь и файловый канал.
Пиксельные байты не считаются текстом AS; формат проверяет сигнатуру, размеры,
флаги, длину и обязательную непрозрачность opaque-буфера. Восстановленный объект
сохраняет методы и фабрики Rectangle/clone без установки стандартных глобальных
объектов в принимающем контексте. Пользовательская финализация не заменяет
внутреннее освобождение пиксельного буфера.

Проверки: standard_bitmap, bitmap_allocation, serialization_validation,
error_allocation. Они проверяют независимость clone, dispose/GC, чтение/запись,
цвета, отсечение, readonly-поля, знаковость getPixel32, перекрытие scroll,
colorTransform, очередь/файл, повреждённый заголовок и отказы выделения памяти.
Файловая проверка пропускается только при явном [FVM_UNSUPPORTED](../include/flashvm/flashvm.h#L48) профиля.

Это частичная реализация, а не завершение BitmapData или этапа 4. Буфер пока
хранит straight RGBA; точное округление premultiplied alpha Flash не подтверждено.
Открыты остальные пиксельные операции, draw, загрузка SWF-ресурсов, attachBitmap,
beginBitmapFill, фильтры, сетевые/AVM1 проверки и полный эталонный корпус.
Ошибочные аргументы/размеры и повторные входы требуют дальнейшей сверки с Flash.

## English

The native flash.display.BitmapData class currently implements bounded dimensions,
read-only size/transparency/rectangle, clone, dispose, pixel access, fillRect,
scroll, colorTransform and copyChannel. getPixel32 returns signed Int32; setters wrap input
colors to Uint32. setPixel preserves alpha. Scroll uses an allocation-before-write
snapshot to avoid overlap smearing and preserves exposed edges. Color transforms
clip their rectangle, clamp channels and retain opaque alpha.

Graph snapshots preserve arbitrary binary pixel data separately from AS text,
validate the canonical header/length/opaque alpha, and restore independent buffers
and captured native factories. Queue and file round trips and clone/restore/scroll
allocation failures are tested by the named suites above. Internal buffer cleanup
is independent of script finalizers.

This is a subset. Straight RGBA storage does not yet establish Flash premultiplied
rounding conformance. Remaining pixel methods, drawing/resource loading, bitmap
fills/attachment, filters, network/AVM1 coverage and full reference acceptance are
open. Stage 4 remains open.

## copyChannel

Каналы задаются масками 1 (R), 2 (G), 4 (B), 8 (alpha). Области источника и
назначения отсекаются совместно, сохраняя относительное смещение. Снимок
выбранного канала предотвращает размазывание при перекрытии. Запись alpha
непрозрачного назначения ничего не меняет. Перед изменением выделяется
временный буфер; отказ выделения сохраняет исходные пиксели. Проверки включены
в standard_bitmap и bitmap_allocation. Дробные координаты и точные правила
преобразования каналов Flash ещё требуют эталонной сверки.

Channel masks select red (1), green (2), blue (4), alpha (8). Joint clipping
preserves source/destination alignment; a selected-channel snapshot prevents
overlap smearing. Opaque destination alpha is unchanged. Allocation failure
precedes pixel mutation. Fractional coordinates and Flash conversion rules
remain subject to reference conformance.

Пока поддерживаются только одиночные маски каналов 1/2/4/8. Комбинации
масок, допускаемые документацией Flash, требуют отдельной сверки и реализации;
в текущем подмножестве другие значения ничего не меняют.
Only single-channel masks are implemented. Combined masks documented by Flash
remain open; other values currently leave the destination unchanged.

## v0.42 copyPixels

Добавлено копирование прямоугольника с необязательным alphaBitmap/alphaPoint и
mergeAlpha. Отсекается пересечение источника, назначения и маски, с сохранением
их смещений. Все исходные пиксели и маска читаются в снимок до записи, включая
совпадение с назначением. Поэтому перекрытие не размазывает изображение, а OOM
не меняет пиксели. Для прозрачного назначения mergeAlpha=false заменяет RGBA;
mergeAlpha=true смешивает source-over. Непрозрачное назначение смешивается с
источником и сохраняет alpha=255. Маска умножает alpha источника.

bitmap_copy_pixels и bitmap_allocation проверяют операции, клиппинг, aliasing,
перенос нативного метода без установки стандартных глобальных объектов, GC
и отказы памяти. Пока используется straight RGBA и целочисленное округление
к ближайшему; точные premultiplied/fixed-point правила Flash не подтверждены.
Дробные координаты, ошибки аргументов и порядок всех getter/coercion вызовов
также требуют полного эталонного корпуса. draw/ресурсы/заливки/фильтры открыты.

copyPixels supports optional alpha bitmap/point and mergeAlpha, joint clipping
and allocation-before-write source/mask snapshots. Transparent replacement and
source-over composition are supported; opaque destinations remain opaque.
The named suites cover overlapping source/mask aliases, transported callbacks,
GC and OOM. Straight RGBA with nearest integer rounding remains an explicit
Flash-conformance limitation; drawing, resources, fills and filters are open.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_UNSUPPORTED` | enum constant | [include/flashvm/flashvm.h:48](../include/flashvm/flashvm.h#L48) |
<!-- source-links:end -->
