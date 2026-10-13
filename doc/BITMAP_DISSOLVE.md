# Pixel dissolve: проверенная модель / Tested dissolve model

v0.64 добавляет `BitmapData.pixelDissolve(source, rect, point, seed, count, color)`.
Это детерминированная модель FlashVM, **не подтверждённая точная реализация
исторической последовательности Flash Player**. Нельзя считать этот метод или
стадию 4 завершёнными только по наличию API и успешным тестам.

Разные source/destination: выбранные пиксели копируются из source без blending.
При source == destination выбранные пиксели заливаются color; point читается,
но его координаты не используются. Прямоугольник ограничивается source и
назначением с согласованным сдвигом. Неположительная/пустая область ничего
не изменяет и возвращает 0. Координаты округляются вниз общей моделью региона.
Opaque destination получает alpha=255; transparent fill с alpha=0 очищает RGB.
Хранилище остаётся straight RGBA, а не точной premultiplied моделью Flash.

Перестановка использует счётчик в ближайшем чётном битовом домене: половины
счётчика L/R отображаются в ((L XOR ((R*R+1) AND mask)), R). Это биекция.
Выходы вне фактической области пропускаются. Возвращённый seed — текущий
счётчик; его следует передавать следующему вызову с той же геометрией.
Отдельные порции вместе равны одному вызову с их суммарным размером до полного
прохода. Каждый пиксель посещается один раз за полный проход, затем цикл повторяется.
Генератор Math.random не используется. Порядок отличается от известных
эвристических эмуляторов и пока не сверен с Flash Player.

Seed/color приводятся к low Uint32, NaN/infinity дают 0. Count должен быть
конечным неотрицательным Number, округляется вниз и ограничивается площадью.
По умолчанию count=floor(clipped area/30), seed/color=0. Явный count=0 не
изменяет пиксели. Параметры преобразуются до получения живого pixel storage;
если getter уничтожил bitmap, возвращается -1. Ошибки getters распространяются.

Output и, для разных bitmap, input целиком готовятся до публикации.
Совместный scratch budget проверяется по memory_limit_bytes. Отказы выделения
памяти не меняют destination. После начала публикации нет allocations/callbacks.
Метод имеет стабильный callback ID 1551 для переноса графов без установки
стандартных классов в принимающем context.

Проверки: bitmap_dissolve содержит literal order/cursor vector, дробление
продолжений, полное покрытие, clipping, default/zero/negative counts, signed seed,
fill/opaque, getter disposal, GC, queue/file/loopback TCP. bitmap_dissolve_allocation
перебирает отказы new и проверяет rollback, GC и balance после teardown.

Открыто: эталонный Flash corpus для pixel order/seed, начального пикселя,
count=0/default, отрицательного count/seed, callback order/ToInt32 clipping,
premultiplied quantization. Документация Adobe задаёт API, но не перестановку:
https://open-flash.github.io/mirrors/as2-language-reference/flash/display/BitmapData.html

## English

v0.64 provides a deterministic FlashVM pixelDissolve model. **Historical Flash
Player pixel order and continuation seeds are not certified.** This API and
stage 4 remain open for reference acceptance.

Distinct bitmaps copy selected source pixels without blending. Same-object
operations fill selected pixels and ignore the coerced destination coordinates.
Aligned clipping uses the common floor-based region model. The bounded balanced
counter permutation is bijective; invalid indexes are skipped. Returned seeds
resume the same geometry and partitioned calls equal one combined call within
a full cycle. Each pixel is visited once per cycle. Math.random is unchanged.

Missing count defaults to floor(clipped area/30); explicit zero performs no
writes. Negative/nonfinite counts are rejected. Seeds/colors use low Uint32
with nonfinite values mapped to zero. Opaque alpha is forced to 255; transparent
zero-alpha fills clear RGB. Storage uses the existing straight RGBA model.
Public coercion finishes before native pixel access is refreshed. Getter-driven
disposal returns -1. Source/output staging and combined scratch limits guarantee
rollback on allocation failures; publication performs no allocations/callbacks.
Callback ID 1551 survives queue/file/TCP graph transport without global installation.

Tests cover literal order/cursor values, continuation partitioning, complete
coverage, clipping, counts/seeds, fill/alpha, disposal, GC, graph transport and
injected allocation failures with teardown balance. Reference pixel order/seeds,
zero/default count quirks, version coercions and premultiplied quantization remain
open. Adobe documentation describes the public API, not its permutation.

Проверено / Verified: headless 239/239, AGG 255/255, Cairo 255/255, portable 230/230; targeted ASan/UBSan 5/5. Leak detection и stack ASan отключены в этой среде. Real Windows/macOS/Android execution and historical Flash equivalence are not certified.
