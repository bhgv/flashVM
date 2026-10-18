# BitmapData.getColorBoundsRect

## Русский

v0.53 добавляет getColorBoundsRect(mask, color, findColor=true). Метод ищет
минимальный axis-aligned Rectangle всех пикселей, у которых (ARGB & mask)
равно color, либо не равно при findColor=false. Маска применяется только
к пикселю; color сравнивается целиком. В текущей straight RGBA модели
сравнение учитывает сохранённый RGB даже при alpha=0. Совпадений нет —
Rectangle(0,0,0,0). Координаты и размеры включают весь последний найденный
пиксель. Каждый результат — новый изменяемый Rectangle.

Mask/color проходят numeric coercion и low-32-bit conversion; нечисловые
NaN/infinity отклоняются. Opaque bitmap использует alpha=255. Disposed
receiver при входе возвращает -1; storage после coercion проверяется снова.
FindColor преобразуется по обычным truth rules, без вызова valueOf объекта.

Сканирование не требует графического backend или отдельной копии пикселей.
Результат строится через Rectangle class, удерживаемый callback capture,
поэтому перенос bitmap в контекст без standard_install сохраняет фабрику,
prototype и GC identity. Stable native callback ID: 1542. Старые IDs не
меняются. Нехватка памяти при создании результата не меняет bitmap;
побочные эффекты пользовательских coercion/prototype callbacks не откатываются.

bitmap_color_bounds.cpp проверяет matching/nonmatching, defaults, alpha/RGB,
маску 0, color outside mask, края/одиночный пиксель/empty results, opaque
low-word -1, независимость Rectangle, queue/GC и disposed/nonfinite случаи.
bitmap_color_bounds_allocation.cpp инъецирует отказы new/new[] фабрики,
проверяет сохранение пикселей, GC и баланс после уничтожения VM.
Профили: headless, AGG, Cairo, portable и heap/global ASan+UBSan.

Открыты точные Flash coercion/premultiplied-alpha edge cases, оставшиеся
BitmapData APIs, filters, текст и медиа. Пункт 4 ROADMAP.md остаётся открытым.

## English

v0.53 adds getColorBoundsRect for equality or inequality of (ARGB & mask)
against the complete color word, with default findColor=true. Results are
independent typed Rectangles enclosing entire matching pixels; an empty
selection returns (0,0,0,0). Current straight storage also compares retained
invisible RGB. Exact Adobe premultiplied behavior remains unverified.

Mask/color use numeric coercion and low-32-bit conversion; nonfinite inputs
are rejected. Opaque alpha is 255; disposed entry receivers return -1 and
storage is revalidated after coercion. The scan is backend-independent.
Retained Rectangle factories/prototypes work after queue transfer without
standard installation. Callback ID 1542 preserves all previous IDs.

Tests cover matching/default/inverted searches, alpha and zero masks,
unmasked color bits, boundaries, empty/single results, independent rectangles,
queue/GC, invalid/disposed receivers and failed result allocations, with
pixel preservation and new/delete balance. Headless/AGG/Cairo/portable and
heap/global ASan+UBSan are checked. User callback side effects are not rolled
back, and remaining conformance/bitmap/filter/text/media work keeps stage 4 open.
