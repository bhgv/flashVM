# Симметричные концы LINESTYLE2 и bevel-соединения

Поддержанные cap/join параметры script lineStyle связаны в [SCRIPT_STROKE_STYLES.ru.md](SCRIPT_STROKE_STYLES.ru.md). Прежние файловые/нативные checkpoints ниже не подтверждают полную расширенную AS2-семантику.

Границы выбранных прямолинейных штрихов описаны в [STROKE_BOUNDS.ru.md](STROKE_BOUNDS.ru.md). Точные кривые и неравномерные/shear метрики открыты; прежняя контрольная точка не подтверждает их полноту.

Геометрический hitTest выбранных стилей описан в [STROKE_HIT_TEST.ru.md](STROKE_HIT_TEST.ru.md); точные bounds и неравномерные/shear метрики остаются открытыми. Исходная контрольная точка рендеринга ниже историческая.

DefineShape4 рисует одинаковые round, butt (без удлинения) или square концы,
сочетая их с round/bevel-соединениями, для сплошных, линейных/фокусных градиентных
и bitmap-обводок. NoClose сохраняет концы при совпадении начальной и конечной точки.
Разные стили начала/конца, miter, pixel hinting и non-scaling возвращают
[FVM_UNSUPPORTED](../include/flashvm/flashvm.h#L48). Резервные коды и биты возвращают [FVM_INVALID_ARGUMENT](../include/flashvm/flashvm.h#L47) раньше
проверки неподдержанных стилей. Усечённые поддержанные записи недопустимы; ошибка
декодирования сохраняет прежний результат.

[fvm_swf_read_shape](../src/swf/shape.cpp#L600) копирует оба селектора в собственное описание paint.
[fvm_swf_shape_build](../src/swf/shape_graph.cpp#L137) публикует необязательные скалярные lineCap и lineJoin;
отсутствующие свойства старых графов означают round. Рендерер читает собственные
целочисленные метаданные без getters. Неверные типы, дробные, бесконечные и
внешние значения возвращают [FVM_INVALID_ARGUMENT](../include/flashvm/flashvm.h#L47) до записи пикселей этой
обводки. Сцена в целом не является транзакцией.

Внутренние селекторы вызова [fvm_path_draw_quality](../src/capi/path_c.cpp#L314),
[fvm_path_mask_width_quality](../src/capi/path_c.cpp#L536), [fvm_path_gradient_focal_quality](../src/capi/gradient_c.cpp#L315) и
[fvm_path_bitmap_paint_quality](../src/capi/bitmap_paint_c.cpp#L210) передают одну геометрию в сплошной цвет, маски
градиента/bitmap и покрытие BitmapData.draw. Аргументы заимствуются на время
синхронного вызова, callbacks не вызываются, глобальный стиль/кеш не сохраняется.
Селекторы проверяются до бэкенда; корректный NO_GRAPHICS возвращает
[FVM_UNSUPPORTED](../include/flashvm/flashvm.h#L48). Прежние контракты подготовки/OOM сохранены. Публичные C89
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


## Итог проверки

Симметричные концы/соединения LINESTYLE2 v0.1253–v0.1266: DefineShape4 рисует одинаковые round/butt/square концы и round/bevel-соединения для сплошных, линейных/фокусных градиентных и bitmap-обводок. Необязательные lineCap/lineJoin сохраняются после GC и переноса графа; отсутствие полей сохраняет round. Селекторы вызова проходят через маски и покрытие BitmapData.draw без изменения публичного C89 ABI, callbacks и resource/wire. Резервные поля недопустимы; разные концы, miter и scaling/hinting явно неподдержаны. Тринадцать новых тестов включают независимую геометрию прямоугольников/круга/bevel, 24 исходных сочетания paint/стиля, 120 ширин, 96 топологий, 96 качеств, неверные нативные селекторы/усечение/неподдержанные стили, метаданные и восстановление round, 24 сцены через сериализацию/канал после удаления отправителя, 24 серии отказов C++-памяти при декодировании/загрузке/paint, 16 перемоток, четыре OS-потока с 96 циклами загрузки/рендеринга/GC, шесть смешанных сцен и 24 банка StateNewStyles. Все 65536 заголовков проходят расширенную политику проверки. Пройдены 170 нативных AGG-тестов, 12 ASan/UBSan и allocation с явным --no-faults, пять SDK-free проверок без графики из нового каталога сборки. Настоящие отказы памяти проверены нативно; sanitizer unwinding ограничен, LeakSanitizer отключён. Проверки исходных ссылок и примеров проходят. Miter limits, разные концы, scaling/hinting, точные bounds/hit tests новых стилей, неравномерные/скошенные правила Flash, Cairo/эталонный растр и устройства/другие платформы остаются открытыми. Все девять этапов открыты. [Контракт](STROKE_CAPS.ru.md).

[paints](../tests/swf_stroke_caps.cpp), [analytic](../tests/swf_stroke_caps_analytic.cpp), [widths](../tests/swf_stroke_caps_widths.cpp), [close](../tests/swf_stroke_caps_close.cpp), [quality](../tests/swf_stroke_caps_quality.cpp), [errors](../tests/swf_stroke_caps_errors.cpp), [metadata](../tests/swf_stroke_caps_metadata.cpp), [transfer](../tests/swf_stroke_caps_transfer.cpp), [allocation](../tests/swf_stroke_caps_allocation.cpp), [timeline](../tests/swf_stroke_caps_timeline.cpp), [threads](../tests/swf_stroke_caps_threads.cpp), [mixed](../tests/swf_stroke_caps_mixed.cpp), [styles](../tests/swf_stroke_caps_styles.cpp).


После изменения внутренних C++-сигнатур и layout используйте новый каталог CMake: старые тестовые объекты могут ссылаться на прежние внутренние сигнатуры рендерера.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_INVALID_ARGUMENT` | enum constant | [include/flashvm/flashvm.h:47](../include/flashvm/flashvm.h#L47) |
| `fvm_path_bitmap_paint_quality` | function | [src/capi/bitmap_paint_c.cpp:210](../src/capi/bitmap_paint_c.cpp#L210) |
| `fvm_path_draw_quality` | function | [src/capi/path_c.cpp:314](../src/capi/path_c.cpp#L314) |
| `fvm_path_gradient_focal_quality` | function | [src/capi/gradient_c.cpp:315](../src/capi/gradient_c.cpp#L315) |
| `fvm_path_mask_width_quality` | function | [src/capi/path_c.cpp:536](../src/capi/path_c.cpp#L536) |
| `fvm_swf_read_shape` | function | [src/swf/shape.cpp:600](../src/swf/shape.cpp#L600) |
| `fvm_swf_shape_build` | function | [src/swf/shape_graph.cpp:137](../src/swf/shape_graph.cpp#L137) |
| `FVM_UNSUPPORTED` | enum constant | [include/flashvm/flashvm.h:48](../include/flashvm/flashvm.h#L48) |
<!-- source-links:end -->
