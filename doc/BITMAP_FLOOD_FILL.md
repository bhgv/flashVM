# BitmapData.floodFill

## Русский

v0.52 добавляет floodFill(x, y, color) независимо от GRAPHICS backend.
Числовые аргументы проходят ActionScript coercion; координаты усекутся к
нулю, вне изображения операция ничего не меняет. Цвет преобразуется в
low 32-bit ARGB; opaque bitmap всегда получает alpha=255. Замещается
четырёхсвязная область с точным совпадением исходного straight ARGB,
включая alpha. Диагональное соседство не соединяет области.

Алгоритм заполняет горизонтальные spans в отдельной копии пикселей.
Соседние строки добавляют один seed на затронутый matching run. Packed
битовая карта не допускает повторной постановки одного seed; stack растёт
по необходимости. Большая однородная область не требует очереди на каждый
пиксель. Учитываются pixel copy, bit map и одновременные old/new stack buffers
при reserve. Это scratch budget, не полная квота live VM/allocator metadata.
При OOM исходная bitmap не меняется; завершённая копия публикуется swap,
сохраняя identity native storage и live bitmap-заливок.

Coercion может вызвать пользовательский valueOf; после него storage
проверяется повторно, dispose не создаёт dangling pixel pointer. GC barrier
защищает receiver и связанные callbacks. User getter side effects не
откатываются. Disposed receiver при входе возвращает -1; disposal во время
coercion даёт INVALID_ARGUMENT. Бесконечные/NaN аргументы отклоняются.
Callback ID 1541 сохраняется при переносе класса/bitmap.

bitmap_flood_fill.cpp проверяет границы/диагонали, alpha-различия,
opaque normalization, усечение/low-word цвет, nonfinite inputs, queue/GC,
реентерабельный dispose и заполнение 128×128 под scratch budget 70000 байтов.
Меньший лимит проверяет rollback; same-color no-op не выделяет scratch.
bitmap_flood_fill_allocation.cpp использует connected comb с многими pending
spans, инъецирует отказы new/new[] включая рост stack после обработанных
строк, проверяет source rollback и баланс после уничтожения VM.
Профили: headless, AGG, Cairo, portable, heap/global ASan+UBSan.

Открыто: полная эталонная coercion/premultiplied-alpha семантика Flash,
оставшиеся BitmapData APIs, фильтры, текст и медиа. Этап 4 не закрыт.

## English

v0.52 adds backend-independent floodFill with ActionScript numeric coercion,
coordinates truncated toward zero, low-32-bit ARGB colors and opaque alpha
normalization. Exact straight ARGB, including alpha, defines a four-connected
component; diagonal neighbors do not connect regions.

Scanline spans are filled in independent pixels. Adjacent matching runs queue
one seed each; a packed seed bit map and dynamically bounded stack avoid a
per-pixel queue for large uniform areas. Scratch budgeting includes copied
pixels, marks and old/new stack buffers during growth, excluding complete
live VM and allocator overhead. Failed allocations preserve the bitmap;
a final swap retains native resource identity and live fill references.

Storage is revalidated after coercion callbacks, and the GC barrier retains
receiver/captures. User side effects are not rolled back. Disposed entry
receivers return -1; reentrant disposal returns INVALID_ARGUMENT. Nonfinite
inputs are rejected. Stable callback ID 1541 survives graph transfer.

Tests cover connectivity/boundaries, alpha differences, opaque normalization,
coordinate/color conversion, invalid inputs, queue/GC, reentrant disposal,
uniform fill under a 70000-byte scratch limit and low-budget rollback.
A connected comb exercises allocation failures during later stack growth,
with unchanged destination and balanced new/delete after VM destruction.
Headless/AGG/Cairo/portable and heap/global ASan+UBSan are checked.
Exact Adobe coercion/premultiplied behavior and remaining bitmap/filter/text/
media features keep stage 4 open.
