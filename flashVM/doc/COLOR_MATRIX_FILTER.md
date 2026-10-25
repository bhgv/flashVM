# BitmapFilter / ColorMatrixFilter

Текущая версия v0.59 также поддерживает [ConvolutionFilter](CONVOLUTION_FILTER.md).
The v0.58 evidence below describes the first filter checkpoint.

## Русский

v0.58 добавляет flash.filters.BitmapFilter и flash.filters.ColorMatrixFilter.
ColorMatrixFilter наследует prototype базового фильтра. clone создаёт новый
объект; color-matrix clone также получает независимые коэффициенты. Матрица
имеет 20 элементов: по строке из четырёх весов RGBA и offset для каждого
выходного канала. Конструктор без матрицы/null/undefined создаёт identity.

matrix getter возвращает новый Array snapshot при каждом чтении. Изменение
исходного массива или getter snapshot не меняет фильтр. Для применения
изменений необходимо присвоить весь массив обратно в matrix. Setter читает
и преобразует 20 публичных индексов, затем строит отдельный private numeric
state и заменяет его одной traced edge. OOM сохраняет прежние коэффициенты;
getter side effects сохраняются. Текущая защитная граница каждого coefficient
±1 000 000; NaN/infinity/missing entries отклоняются. Полная Flash coercion
семантика и внепороговые коэффициенты ещё требуют эталонной проверки.

В v0.58 BitmapData.applyFilter поддерживал только ColorMatrixFilter. Геометрия и
source/destination clipping описаны в BITMAP_THRESHOLD.md. Весь source ROI
сначала копируется, затем вычисляется dot product строки матрицы и straight
RGBA pixel, прибавляется offset, результат ограничивается 0..255 и округляется
к ближайшему целому. Opaque destination сохраняет alpha=255; при alpha=0 RGB
результата очищается. Нет новых callbacks/allocations после snapshot, поэтому
перекрытие source/destination и отказ памяти не дают частичных записей.
Неподдерживаемые фильтры возвращают [FVM_UNSUPPORTED](../include/flashvm/flashvm.h#L48), а не identity fallback.

generateFilterRect(rect, colorMatrixFilter) возвращает независимый Rectangle
без расширения границ: color matrix не увеличивает геометрию. Фабрики Array,
Rectangle и фильтров удерживаются captured graph и работают после переноса
без standard_install в принимающем контексте. Callback IDs: filters 1600..1605,
applyFilter 1547, generateFilterRect 1548. Resource kinds 54..57 append-only.
Private matrix graph валидируется перед захватом и до восстановления: 20
numeric fields, finite bounded coefficients, без prototypes/metatables/callbacks.

color_matrix_filter.cpp проверяет inheritance/instanceof, identity, независимость
матриц и clone, channel swap/offset/clamp/alpha, opaque storage, shifted alias,
output rectangle, queue/file/loopback TCP, GC, quotas, getter disposal, invalid
assignments и повреждённые serialized coefficients. color_matrix_filter_allocation.cpp
инъецирует failures конструктора, setter/getter/clone/apply и deserialization,
сверяет прежние коэффициенты/пиксели и баланс new/delete после уничтожения VM.
Учебный C89-пример examples/embedding/color_matrix_filter/main.c объясняет каждую
строку и явно проверяет перестановку каналов без assert.
Профили: headless, AGG, Cairo, portable; heap/global ASan+UBSan, без stack ASan
и leak scanner в этом окружении. Общие suites повторяются после расширения
standard installation и captured graph.

Открыто: pixel rounding/premultiplied quantization и точная Flash-сверка,
другие восемь фильтров Flash 8, MovieClip.filters/cacheAsBitmap, SWF filter
records, clipping/group rendering, полный текст и медиа. Этап 4 не завершён.

## English

v0.58 adds BitmapFilter and ColorMatrixFilter with prototype inheritance and
independent cloning. ColorMatrixFilter defaults to identity. Its matrix getter
returns a fresh Array; original arrays and returned snapshots never change native
coefficients without reassignment. The setter converts all twenty public indices,
then atomically installs independent private numeric storage. Allocation failures
preserve the old matrix; getter side effects remain. Current safety bounds are
±1,000,000 per coefficient; missing/NaN/infinite entries are rejected.

The v0.58 BitmapData.applyFilter supported ColorMatrixFilter only. It snapshots
the aligned source ROI before writing, computes straight RGBA dot products plus
offsets, clamps to bytes and rounds to the nearest integer. Opaque alpha stays
255 and zero alpha clears output RGB. No callbacks/allocations occur after the
snapshot. Unsupported filters return [FVM_UNSUPPORTED](../include/flashvm/flashvm.h#L48). generateFilterRect returns
an independent Rectangle with unchanged color matrix bounds.

Captured filter/Array/Rectangle factories survive queue/file/network graph transport
without local standard installation. Append-only callback IDs are 1600..1605 and
1547..1548; kinds are 54..57. Private numeric graph validation rejects malformed
fields, out-of-range coefficients and callback/prototype/metatable substitution.
Tests cover copies/inheritance, fixed pixel arithmetic, overlap, quotas, getter
disposal, queue/file/loopback TCP/GC, malformed serialized storage and injected
constructor/setter/getter/clone/apply/deserialization failures with allocation balance.
The line-commented C89 embedding tutorial verifies a red/blue swap without assertions.
Headless, AGG, Cairo, portable and heap/global ASan+UBSan are exercised; stack
ASan and leak scanning are disabled. Exact Adobe rounding/coercion/premultiplied
conformance, other filters, MovieClip filter rendering, SWF filter records and
remaining text/media keep stage 4 open.

API reference: [Adobe AS2 ColorMatrixFilter mirror](https://open-flash.github.io/mirrors/as2-language-reference/flash/filters/ColorMatrixFilter.html).

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_UNSUPPORTED` | enum constant | [include/flashvm/flashvm.h:48](../include/flashvm/flashvm.h#L48) |
<!-- source-links:end -->
