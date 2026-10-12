# BitmapData: текущая реализация / Current implementation

Реализован класс `flash.display.BitmapData`: ограниченные размеры 1–2880,
прозрачные/непрозрачные буферы, width/height/transparent/rectangle только для
чтения, clone, dispose, getPixel, getPixel32, setPixel, setPixel32, fillRect,
scroll и colorTransform. getPixel32 возвращает знаковое Int32; значения цветов
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
Файловая проверка пропускается только при явном FVM_UNSUPPORTED профиля.

Это частичная реализация, а не завершение BitmapData или этапа 4. Буфер пока
хранит straight RGBA; точное округление premultiplied alpha Flash не подтверждено.
Открыты остальные пиксельные операции, draw, загрузка SWF-ресурсов, attachBitmap,
beginBitmapFill, фильтры, сетевые/AVM1 проверки и полный эталонный корпус.
Ошибочные аргументы/размеры и повторные входы требуют дальнейшей сверки с Flash.

## English

The native flash.display.BitmapData class currently implements bounded dimensions,
read-only size/transparency/rectangle, clone, dispose, pixel access, fillRect,
scroll and colorTransform. getPixel32 returns signed Int32; setters wrap input
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
