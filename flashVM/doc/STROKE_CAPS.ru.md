# Симметричные концы LINESTYLE2 и bevel-соединения

DefineShape4 рисует одинаковые round, butt (без удлинения) или square концы,
сочетая их с round/bevel-соединениями, для сплошных, линейных/фокусных градиентных
и bitmap-обводок. NoClose сохраняет концы при совпадении начальной и конечной точки.
Разные стили начала/конца, miter, pixel hinting и non-scaling возвращают
[FVM_UNSUPPORTED](../include/flashvm/flashvm.h#L46). Резервные коды и биты возвращают [FVM_INVALID_ARGUMENT](../include/flashvm/flashvm.h#L45) раньше
проверки неподдержанных стилей. Усечённые поддержанные записи недопустимы; ошибка
декодирования сохраняет прежний результат.

[fvm_swf_read_shape](../src/swf/shape.cpp#L469) копирует оба селектора в собственное описание paint.
[fvm_swf_shape_build](../src/swf/shape_graph.cpp#L117) публикует необязательные скалярные lineCap и lineJoin;
отсутствующие свойства старых графов означают round. Рендерер читает собственные
целочисленные метаданные без getters. Неверные типы, дробные, бесконечные и
внешние значения возвращают [FVM_INVALID_ARGUMENT](../include/flashvm/flashvm.h#L45) до записи пикселей этой
обводки. Сцена в целом не является транзакцией.

Внутренние селекторы вызова [fvm_path_draw_quality](../src/capi/path_c.cpp#L316),
[fvm_path_mask_width_quality](../src/capi/path_c.cpp#L499), [fvm_path_gradient_focal_quality](../src/capi/gradient_c.cpp#L240) и
[fvm_path_bitmap_paint_quality](../src/capi/bitmap_paint_c.cpp#L155) передают одну геометрию в сплошной цвет, маски
градиента/bitmap и покрытие BitmapData.draw. Аргументы заимствуются на время
синхронного вызова, callbacks не вызываются, глобальный стиль/кеш не сохраняется.
Селекторы проверяются до бэкенда; корректный NO_GRAPHICS возвращает
[FVM_UNSUPPORTED](../include/flashvm/flashvm.h#L46). Прежние контракты подготовки/OOM сохранены. Публичные C89
функции/структуры сохраняют ABI и round по умолчанию.

AGG использует butt_cap, round_cap, square_cap и bevel_join; Cairo — соответствующие
enum. Приёмка этой серии использует нативный AGG и SDK-free профиль без графики.
Cairo, устройства и эталонный растр учитываются отдельно. Нового AS2-метода или
политики преобразования скриптовых аргументов не добавлено.

## Проверки и оставшийся объём

Независимые геометрические проверки требуют точные прямоугольники butt/square,
частичное покрытие круглого угла и срез bevel вместо round. Поверхности проверяются
для 24 сочетаний стиля/paint, 120 ширин, 96 топологий и 96 качеств. Проверены
неверные скаляры, восстановление round, 24 сцены через сериализацию/канал, отказы
памяти, 16 перемоток, четыре потока с 96 циклами загрузки/рендеринга/GC, порядок
заливки/обводки и 24 новых банка. Тест всех 65536 заголовков LINESTYLE2 учитывает
расширенную область допустимых стилей.

Miter limits, разные концы, hinting/non-scaling, точные bounds/hit-test для
некруглых стилей, неравномерные/скошенные обводки Flash, эталонное округление и
устройства/другие платформы остаются открытыми. Все девять этапов открыты;
расширен только файловый рендеринг.

Первоисточник: Adobe SWF specification, LINESTYLE2:
https://open-flash.github.io/mirrors/swf-spec-19.pdf

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_INVALID_ARGUMENT` | enum constant | [include/flashvm/flashvm.h:45](../include/flashvm/flashvm.h#L45) |
| `fvm_path_bitmap_paint_quality` | function | [src/capi/bitmap_paint_c.cpp:155](../src/capi/bitmap_paint_c.cpp#L155) |
| `fvm_path_draw_quality` | function | [src/capi/path_c.cpp:316](../src/capi/path_c.cpp#L316) |
| `fvm_path_gradient_focal_quality` | function | [src/capi/gradient_c.cpp:240](../src/capi/gradient_c.cpp#L240) |
| `fvm_path_mask_width_quality` | function | [src/capi/path_c.cpp:499](../src/capi/path_c.cpp#L499) |
| `fvm_swf_read_shape` | function | [src/swf/shape.cpp:469](../src/swf/shape.cpp#L469) |
| `fvm_swf_shape_build` | function | [src/swf/shape_graph.cpp:117](../src/swf/shape_graph.cpp#L117) |
| `FVM_UNSUPPORTED` | enum constant | [include/flashvm/flashvm.h:46](../include/flashvm/flashvm.h#L46) |
<!-- source-links:end -->
