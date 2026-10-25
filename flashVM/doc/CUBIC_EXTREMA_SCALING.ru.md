# Экстремумы малых кубических Bezier

При достаточно малых координатах квадрат коэффициента производной и произведение других коэффициентов округлялись до нуля. Дискриминант тогда терял два внутренних корня; [`fvm_path_bounds`](../include/flashvm/path.h#L97) и round/round stroke bounds могли занизить границы. Регрессия native cubic с controls 0,12,-12,0, масштабированными на 2^-600, падает на v0.1464. Независимые аналитические экстремумы нормализованной кривой равны ±2√3.

В кубической ветке выбирается максимальный модуль трёх коэффициентов производной. Если его квадрат ниже минимального нормального double, frexp задаёт общий показатель, а ldexp увеличивает все коэффициенты одной двоичной степенью до вычисления дискриминанта. На обычной двоичной арифметике это точное масштабирование коэффициентов и не меняет математические корни. Прежняя устойчивая q-формула сохранена. Нормальные коэффициенты и линейная/quadratic ветка не меняют порядок вычислений. Epsilon не добавляется.

## Контракт и ограничения

[`fvm_path_bounds`](../include/flashvm/path.h#L97) заимствует неизменяемый native path и публикует четыре scalar-координаты в принадлежащий вызывающему [`FVM_PathBounds`](../include/flashvm/path.h#L50) при [FVM_OK](../include/flashvm/flashvm.h#L44). Неверные path/out дают [FVM_INVALID_ARGUMENT](../include/flashvm/flashvm.h#L47) без публикации. Нет выделений, callbacks, VM, GC или устройств. Native path принадлежит вызывающему до [`fvm_path_destroy`](../include/flashvm/path.h#L64); время жизни и отсутствие совместной мутации обеспечиваются владельцем. [`fvm_path_transform`](../include/flashvm/path.h#L87) и [`fvm_path_append`](../include/flashvm/path.h#L91) сохраняют прежние транзакционные проверки и пределы координат/матриц ±1 000 000. Round/round внутренняя геометрия использует тот же helper, затем расширяет границы на половину положительной ширины; её прежние статусы и требования к двум output сохраняются.

Cubic остаётся native path-командой: managed MovieClip по-прежнему принимает quadratic. Pixel raster и native JIT не получают нового алгоритма. Общая почти кратная/почти коллинеарная double-геометрия и потеря младших разрядов при построении коэффициентов из больших близких координат не получают arbitrary-precision решения. Требуется обычная двоичная арифметика; FTZ/fast-math и физические платформы не проверены. Все девять этапов плана остаются открытыми; кодеки, дочерние потоки и полная Flash-reference приёмка этой серией не закрываются.

## Проверки и пример

Независимый oracle вычисляет корни обычной long-double формулой на немасштабированных fixture-controls и оценивает curve через de Casteljau. Это дополняется точными аналитическими ±2√3 и 16/3, а также 1025 выборками на каждую из семи форм. Проверяются оси, диагональ, отражение, показатели до -1000, переходы subnormal квадратов, round-обводка, quadratic/linear и кратный/отрицательный дискриминант, append/transform, несколько контуров и четыре независимых OS worker с 196 проверками обводки. Формулы oracle не извлекаются из production helper.

[Исполняемый C89-пример](../examples/embedding/cubic_extrema/README.ru.md) создаёт native path, проверяет обычные bounds, масштабирует реальный public path API на 2^-600, нормализует новые bounds и освобождает owner без VM. Каждая строка объяснена; ошибки обычные, без assert. Пример работает и без графики. Пять API и их владение/статусы подробно описаны в README. Публичный C89 ABI, callback ID и форматы остаются прежними.


## Проверки v0.1484

Пройдены 378 нативных executable-проверок прямым запуском, 132 CTest без графики и 132 CTest ASan/UBSan. LeakSanitizer отключён. CTest нативного профиля не запустил несколько файлов из-за прав доступа в workspace; для полного набора правa восстанавливались непосредственно перед каждым прямым запуском. Все 378 завершились с кодом 0. Оба потребителя изменённого inline-header принудительно пересобраны перед итоговой линковкой; устаревшие результаты промежуточных сборок не засчитаны. Пройдены source-link, per-line C89 example/API-table/catalog проверки. Это не тест реального устройства или полного SWF ≤ 8.

| Fixture | Scope / Область |
| --- | --- |
| [path_cubic_scale_analytic](../tests/path_cubic_scale_analytic.cpp) | Analytic symmetric two-extrema oracle |
| [path_cubic_scale_hump](../tests/path_cubic_scale_hump.cpp) | Single-hump analytic 16/3 oracle |
| [path_cubic_scale_horizontal](../tests/path_cubic_scale_horizontal.cpp) | Horizontal axis and reflection |
| [path_cubic_scale_vertical](../tests/path_cubic_scale_vertical.cpp) | Vertical axis and reflection |
| [path_cubic_scale_diagonal](../tests/path_cubic_scale_diagonal.cpp) | Two scaled axes |
| [path_cubic_scale_scales](../tests/path_cubic_scale_scales.cpp) | Uniform scale range |
| [path_cubic_scale_transition](../tests/path_cubic_scale_transition.cpp) | Normal/subnormal-square transition |
| [path_cubic_scale_round](../tests/path_cubic_scale_round.cpp) | Round/round expanded extents |
| [path_cubic_scale_quadratic](../tests/path_cubic_scale_quadratic.cpp) | Quadratic derivative control |
| [path_cubic_scale_linear](../tests/path_cubic_scale_linear.cpp) | Linear cubic derivative control |
| [path_cubic_scale_repeated](../tests/path_cubic_scale_repeated.cpp) | Repeated derivative root control |
| [path_cubic_scale_monotone](../tests/path_cubic_scale_monotone.cpp) | Negative discriminant control |
| [path_cubic_scale_append](../tests/path_cubic_scale_append.cpp) | Native append with tiny affine scale |
| [path_cubic_scale_transform](../tests/path_cubic_scale_transform.cpp) | Transactional path transform |
| [path_cubic_scale_contours](../tests/path_cubic_scale_contours.cpp) | Union of translated contours |
| [path_cubic_scale_sampling](../tests/path_cubic_scale_sampling.cpp) | 1025 independent de Casteljau samples |
| [path_cubic_scale_threads](../tests/path_cubic_scale_threads.cpp) | Four OS workers and 196 stroke checks |


## Перенесённые controls

Близкие перенесённые controls теперь используют точное локальное вычитание перед вычислением производной и однократное восстановление origin. См. [контракт и ограничения](TRANSLATED_CURVE_EXTREMA.ru.md). Это продолжает исправление малого discriminant без заявления полной численной совместимости.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_INVALID_ARGUMENT` | enum constant | [include/flashvm/flashvm.h:47](../include/flashvm/flashvm.h#L47) |
| `FVM_OK` | enum constant | [include/flashvm/flashvm.h:44](../include/flashvm/flashvm.h#L44) |
| `fvm_path_append` | function | [include/flashvm/path.h:91](../include/flashvm/path.h#L91) |
| `fvm_path_bounds` | function | [include/flashvm/path.h:97](../include/flashvm/path.h#L97) |
| `fvm_path_destroy` | function | [include/flashvm/path.h:64](../include/flashvm/path.h#L64) |
| `fvm_path_transform` | function | [include/flashvm/path.h:87](../include/flashvm/path.h#L87) |
| `FVM_PathBounds` | struct | [include/flashvm/path.h:50](../include/flashvm/path.h#L50) |
<!-- source-links:end -->
