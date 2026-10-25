# Публичные round-запросы для малых обводок

## Исправленное поведение

Публичный [`fvm_path_hit_test`](../include/flashvm/path.h#L103) возводил малые длины касательных и радиусы в квадрат. Для линии (0,0)..(10s,0), ширины 2s и s=2^-600 точка (5s,2s) ошибочно считалась попавшей: квадрат расстояния и квадрат радиуса округлялись до нуля. Отдельный executable, связанный с v0.1504, воспроизводит assertion failure до исправления.

Только публичные round-запросы с положительным радиусом, у которых квадрат нормы касательной или радиуса ниже DBL_MIN, теперь используют масштабированный расчёт. Касательная нормализуется по максимальной компоненте, продольное расстояние ограничивается конечным отрезком, остаточное расстояние сравнивается через ограниченные отношения к радиусу. Отсечение по компонентам выполняется до деления. Постоянный stroked segment сохраняет прежнюю семантику диска. Обычные normal-square запросы, fills, adaptive subdivision и private cap/join запросы сохраняют прежние пути.

## Контракт публичного API

[`fvm_path_hit_test`](../include/flashvm/path.h#L103) заимствует неизменяемый native path и записывает целочисленный boolean в память вызывающей стороны. Координаты запроса конечны в пределах +/-2000000, width конечна в 0..1000000, even_odd равен 0 или 1, path/output ненулевые. Недопустимые аргументы возвращают [FVM_INVALID_ARGUMENT](../include/flashvm/flashvm.h#L47); допустимый запрос — [FVM_OK](../include/flashvm/flashvm.h#L44). Нулевая ширина выбирает неявно замкнутые fills с документированным boundary tolerance; положительная — round strokes. Для кривых сохраняются adaptive chord tolerance 0.0001 и глубина до 20. Выделений, callbacks, VM execution, GC и передачи владения нет. Независимые paths допускают параллельные запросы; изменение запрашиваемого path требует синхронизации вызывающей стороны. Другие ошибки обхода передаются без публикации успешного результата.

Существующий [пример C89 cubic](../examples/embedding/cubic_extrema/README.ru.md) демонстрирует владение native path и границы. Он не демонстрирует этот hit query; его численные регрессии проверяются отдельными fixtures ниже.

## Ограничения

Ширины, у которых половина сама округляется до нуля, остаются вне этого расширения. Округление около границы, потерянные разности координат и минимальные subnormal projection distances не объявляются точными predicates. Epsilon не добавляется. Rasterization, SWF version visibility, JIT emission, публичные заголовки и serialized formats не меняются. Приёмка reference/device/Cairo и все девять этапов roadmap открыты.

## Fixtures

Семнадцать независимых executable fixtures проверяют горизонтальные, вертикальные и диагональные отрезки, отражения, постоянные отрезки, масштабы до 2^-1000, переход normal/subnormal квадратов, обычные диапазоны, публичные transactional transform/append, оба торца, далёкие точки, малый радиус на обычном отрезке, широкий диск около малого отрезка, матрицу сочетаний и четыре native OS workers с независимыми paths. Ожидания следуют из аналитических расстояний до тела и торцов; private selected-stroke query не используется как oracle.


## Контрольная точка v0.1524

Все 360 выбранных executable no-graphics Release и 182 no-graphics ASan/UBSan Debug завершаются нулём при включённых assertions. Оба профиля включают семнадцать новых fixtures, прежние tiny/translated curve и selected-stroke регрессии, публичные path queries, affine/Point geometry и существующие C89 tutorials. Release также включает дополнительные sound/stream/mix регрессии; это свидетельство регрессий, а не новая функциональность аудио. LeakSanitizer отключён; предупреждения поиска имени sanitizer executable могут ограничивать символизацию. Device, Cairo и новая native-JIT приёмка не заявляются.

Серия format-patch из двадцати коммитов проверяется через git am --3way на чистой опубликованной базе v0.1504 со сверкой итоговых деревьев, последовательных версий и обеих подписей. Публичные заголовки не меняются. Source links и frontend tutorial checks проходят; generated logs остаются вне Git. Все девять этапов roadmap открыты.

| Fixture / Проверка | Source / Исходник |
| --- | --- |
| analytic | [analytic](../tests/path_public_round_scale_analytic.cpp) |
| vertical | [vertical](../tests/path_public_round_scale_vertical.cpp) |
| diagonal | [diagonal](../tests/path_public_round_scale_diagonal.cpp) |
| negative | [negative](../tests/path_public_round_scale_negative.cpp) |
| zero segment | [zero_segment](../tests/path_public_round_scale_zero_segment.cpp) |
| scales | [scales](../tests/path_public_round_scale_scales.cpp) |
| transition | [transition](../tests/path_public_round_scale_transition.cpp) |
| normal | [normal](../tests/path_public_round_scale_normal.cpp) |
| tiny zero | [tiny_zero](../tests/path_public_round_scale_tiny_zero.cpp) |
| transform | [transform](../tests/path_public_round_scale_transform.cpp) |
| append | [append](../tests/path_public_round_scale_append.cpp) |
| caps | [caps](../tests/path_public_round_scale_caps.cpp) |
| far | [far](../tests/path_public_round_scale_far.cpp) |
| thin | [thin](../tests/path_public_round_scale_thin.cpp) |
| wide | [wide](../tests/path_public_round_scale_wide.cpp) |
| matrix | [matrix](../tests/path_public_round_scale_matrix.cpp) |
| threads | [threads](../tests/path_public_round_scale_threads.cpp) |

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_INVALID_ARGUMENT` | enum constant | [include/flashvm/flashvm.h:47](../include/flashvm/flashvm.h#L47) |
| `FVM_OK` | enum constant | [include/flashvm/flashvm.h:44](../include/flashvm/flashvm.h#L44) |
| `fvm_path_hit_test` | function | [include/flashvm/path.h:103](../include/flashvm/path.h#L103) |
<!-- source-links:end -->
