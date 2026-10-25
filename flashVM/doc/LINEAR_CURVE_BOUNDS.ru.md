# Сертифицированные линейные Bezier-штрихи

Текущее расширение: [адаптивные векторы](ADAPTIVE_CURVE_BOUNDS.ru.md). Прежнее требование абсолютной сетки ниже историческое.

Точные bounds теперь используют геометрию отрезка для quadratic/cubic с коллинеарными controls внутри прямоугольника концов. Для неосевого пути все координаты должны точно принадлежать сетке 1/16 и оставаться в исходном домене ±1 000 000; требуются FLT_RADIX=2 и DBL_MANT_DIG ≥53. Умножение на 16 даёт точные целые, разности не превышают 32 000 000, произведения меньше 2^50, их разности меньше 2^51. Проверка cross product поэтому не теряет ненулевое отклонение из-за округления. Epsilon не используется. Домен публичного path API не расширен; предел вынесен в общий [заголовок](../src/capi/path_geometry_constants.h).

Для горизонтальных/вертикальных путей сохраняются произвольные допустимые конечные координаты без требования сетки. Controls должны быть внутри интервала концов, но cubic-controls теперь могут быть неупорядоченными. После нормализации u,v∈[0,1] производная кубической координаты линейна по u,v; в четырёх вершинах квадрата она пропорциональна t², 2t(1-t), (1-2t)² и (1-t)². Все выражения неотрицательны; поэтому весь такой класс монотонен. В частности, controls в противоположных концах дают допустимую нулевую производную в середине.

## Контракт

[fvm_path_stroke_bounds_style](../src/capi/path_hit_c.cpp#L733) сохраняет существующий контракт: неизменяемый заимствованный путь, положительная конечная ширина ≤1 000 000, поддержанные cap/join, два обязательных принадлежащих вызывающему результата. Проверка и обход не выделяют память, не вызывают callbacks/GC и не требуют контекста. Ошибки аргументов и неподдержанные кривые сохраняют оба результата. Совместная мутация пути запрещена. Используются прежние конечные тела, round/none/square торцы, round/bevel joins и Close. Constant и одиночный Move не добавляют штрих.

Внеосевые координаты вне сетки, отклонения от коллинеарности и controls вне интервала не сокращаются; для non-round сохраняется [FVM_UNSUPPORTED](../include/flashvm/flashvm.h#L48) с managed fallback. Round/round сохраняет общие аналитические кривые. Поворот или масштаб могут вывести путь с сетки. Неравномерная метрика ширины остаётся прежней.

Managed quadratic автоматически использует новые bounds в getBounds, размерах и hitTest без shape. getRect и смешанная заливка сохраняют прежний учёт исходной рамки. Cubic остаётся нативной возможностью; MovieClip его не принимает. Пиксели, shape-hit subdivision, C89 ABI, wire и callback ID не меняются. [C89-пример](../examples/embedding/stroke_styles/README.ru.md) добавляет настоящий диагональный curveTo с none/bevel и игнорируемым одиночным moveTo.

Общие non-round кривые, произвольная точная collinearity вне сетки, неравномерный масштаб/shear, miter/hinting/non-scaling, полная Flash-reference-приёмка, Cairo и устройства остаются открытыми. Все девять этапов roadmap открыты.

## Проверенная контрольная точка v0.1400

21 локальный коммит v0.1380–v0.1400 поверх опубликованного master v0.1379. Пройдены 307 выбранных нативных AGG/JIT-тестов, 61 тест интерпретатора без графики и 61 ASan/UBSan, включая окончательный C89-пример и vector_paths. LeakSanitizer отключён. Восемнадцать новых fixtures проверяют quadratic/cubic, шесть cap/join-стилей, направление, неупорядоченные axis-controls, дробную сетку, поворот/масштаб, предельные координаты и ширину, три отказа с обоими неизменёнными результатами, контуры, joins/Close, managed geometry/hairline/mixed/dimensions/hitTest, 96 quality/paint/style комбинаций, 24 графа через binary/channel после удаления отправителя и четыре OS-потока с 96 bounds/GC эпохами. Существующий набор сохраняет JIT, таймлайн и allocation-регрессии, не означая полной новой reference-приёмки каждой подсистемы. Source/example/diff checks пройдены. Cairo и устройства не проверялись; все девять этапов открыты.

| Fixture | Проверка |
| --- | --- |
| [path_linear_curve_quadratic](../tests/path_linear_curve_quadratic.cpp) | Коллинеарные quadratic |
| [path_linear_curve_cubic](../tests/path_linear_curve_cubic.cpp) | Неупорядоченные collinear cubic |
| [path_linear_curve_reverse](../tests/path_linear_curve_reverse.cpp) | Обратное направление |
| [path_linear_curve_unordered_axis](../tests/path_linear_curve_unordered_axis.cpp) | Стационарная середина cubic |
| [path_linear_curve_grid](../tests/path_linear_curve_grid.cpp) | Дробная двоичная сетка |
| [path_linear_curve_rotation](../tests/path_linear_curve_rotation.cpp) | Поворот и масштаб |
| [path_linear_curve_limits](../tests/path_linear_curve_limits.cpp) | Пределы координат и ширины |
| [path_linear_curve_errors](../tests/path_linear_curve_errors.cpp) | Отказы без изменения результатов |
| [path_linear_curve_contours](../tests/path_linear_curve_contours.cpp) | Изолированные Move |
| [path_linear_curve_joins](../tests/path_linear_curve_joins.cpp) | Соединения и замыкание |
| [movieclip_linear_curve_geometry](../tests/movieclip_linear_curve_geometry.cpp) | Managed getBounds/getRect |
| [movieclip_linear_curve_hairline](../tests/movieclip_linear_curve_hairline.cpp) | Hairline |
| [movieclip_linear_curve_mixed](../tests/movieclip_linear_curve_mixed.cpp) | Смешанная заливка |
| [movieclip_linear_curve_dimensions](../tests/movieclip_linear_curve_dimensions.cpp) | Размеры MovieClip |
| [movieclip_linear_curve_hitbox](../tests/movieclip_linear_curve_hitbox.cpp) | Bounding hitTest |
| [movieclip_linear_curve_quality](../tests/movieclip_linear_curve_quality.cpp) | Качество и виды paint |
| [movieclip_linear_curve_transfer](../tests/movieclip_linear_curve_transfer.cpp) | Binary/channel после teardown |
| [movieclip_linear_curve_threads](../tests/movieclip_linear_curve_threads.cpp) | Независимые потоки и GC |

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_path_stroke_bounds_style` | function | [src/capi/path_hit_c.cpp:733](../src/capi/path_hit_c.cpp#L733) |
| `FVM_UNSUPPORTED` | enum constant | [include/flashvm/flashvm.h:48](../include/flashvm/flashvm.h#L48) |
<!-- source-links:end -->
