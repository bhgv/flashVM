# Адаптивные точные векторы линейных Bezier

Current tiny-tangent extension / Расширение для коротких касательных: [EN](SCALED_STROKE_TANGENTS.md) / [RU](SCALED_STROKE_TANGENTS.ru.md). The squared-norm restriction below is historical.

Для non-round bounds больше не требуется фиксированная абсолютная сетка 1/16. Начало пути вычитается из конца и controls с проверкой остатка Two-Diff. При ненулевом остатке сокращение отклоняется. Каждая промежуточная величина записывается в volatile double, чтобы не полагаться на расширенные регистры. Математическая конструкция описана в [работе Shewchuk](https://www.cs.cmu.edu/~quake/robust.html); требуется обычное округление к ближайшему и стандартная двоичная арифметика без fast-math/flush-to-zero.

Общий span конца определяет показатель frexp; ldexp увеличивает все точные разности так, чтобы их модули были меньше 2^25. Все масштабированные компоненты должны быть целыми. Произведения тогда меньше 2^50, разность cross-products меньше 2^51 и точна при мантиссе ≥53. Epsilon нет. Проверка radix=2 и точности сохраняется. Контроль переполнения показателя использует INT_MAX. Домен координат остаётся ±1 000 000, поэтому масштабирование увеличивает величину и не теряет мелкие компоненты. Контуры с большим диапазоном разрядов могут остаться неподдержанными.

## Контракт и ограничения

[fvm_path_stroke_bounds_style](../src/capi/path_hit_c.cpp#L529) по-прежнему заимствует неизменяемый путь и требует положительную конечную width ≤1 000 000, допустимые selectors и два принадлежащих вызывающему результата. Проверка не выделяет память, не вызывает callbacks/GC и не требует VM. Ошибки и unsupported сохраняют оба результата; совместная мутация пути запрещена. Оси сохраняют прежний быстрый путь. Для неосевого сокращения дополнительно проверяется ненулевая норма, чтобы не превращать старый fallback в пустой штрих при underflow существующего segment walker.

Controls должны находиться внутри прямоугольника концов и быть точно коллинеарными после масштабирования. Неупорядоченные cubic-controls внутри отрезка поддержаны как раньше. Потерянный остаток, дробные масштабированные компоненты, ненулевая ориентация и ненормализуемый тангенс сохраняют [FVM_UNSUPPORTED](../include/flashvm/flashvm.h#L46) для non-round. Round/round сохраняет общий аналитический расчёт. Конечные тела, торцы, joins и Close не меняются.

Managed quadratic использует результат в getBounds, размерах и hitTest без shape; getRect и mixed-fill сохраняют исходную рамку. Cubic остаётся нативным. Пиксели, shape-hit subdivision, публичный C89 ABI, wire и callback ID не меняются. [C89-пример](../examples/embedding/stroke_styles/README.ru.md) использует настоящий компактный curveTo вне прежней сетки, явно заменяя GC-корень каждого результата.

Это ограниченный точный предикат, не полный adaptive-precision orientation. Общие non-round кривые, все произвольные double-векторы, новая метрика nonuniform/shear, miter/hinting/non-scaling, полная Flash/reference-приёмка, Cairo и физические платформы остаются открытыми. Все девять этапов roadmap открыты.

## Проверенная контрольная точка v0.1421

21 локальный коммит v0.1401–v0.1421 поверх опубликованного master v0.1400. Пройдены 325 выбранных нативных AGG/JIT-тестов, 79 тестов без графики и 79 ASan/UBSan с окончательным C89-примером и vector_paths; LeakSanitizer отключён. Восемнадцать новых fixtures проверяют оба порядка кривых, шесть стилей, направление, перенос до 999000, двоичные масштабы и нормализованную геометрию при масштабе 2^-400, сохранность обоих результатов при потерянном остатке/underflow/избыточной разрядности, контуры, managed geometry/hairline/mixed/dimensions/hitTest, 96 сочетаний качества/paint/style, 24 графа через binary/channel после удаления отправителя и четыре OS-потока с 96 bounds/GC эпохами. Существующие JIT/таймлайн/allocation тесты являются регрессиями, а не новой полной reference-приёмкой подсистем. Пройдены source/example/diff checks. Другие представления double, режимы округления, Cairo и физические устройства не проверялись; все девять этапов открыты.

| Fixture | Проверка |
| --- | --- |
| [path_adaptive_curve_quadratic](../tests/path_adaptive_curve_quadratic.cpp) | Компактные quadratic |
| [path_adaptive_curve_cubic](../tests/path_adaptive_curve_cubic.cpp) | Компактные cubic |
| [path_adaptive_curve_reverse](../tests/path_adaptive_curve_reverse.cpp) | Обратное направление |
| [path_adaptive_curve_translation](../tests/path_adaptive_curve_translation.cpp) | Большие и дробные переносы |
| [path_adaptive_curve_scales](../tests/path_adaptive_curve_scales.cpp) | Двоичные масштабы |
| [path_adaptive_curve_small](../tests/path_adaptive_curve_small.cpp) | Малые нормализуемые тангенсы |
| [path_adaptive_curve_subtraction](../tests/path_adaptive_curve_subtraction.cpp) | Потеря остатка вычитания |
| [path_adaptive_curve_underflow](../tests/path_adaptive_curve_underflow.cpp) | Underflow нормы тангенса |
| [path_adaptive_curve_precision](../tests/path_adaptive_curve_precision.cpp) | Избыточная разрядность |
| [path_adaptive_curve_contours](../tests/path_adaptive_curve_contours.cpp) | Изолированные Move |
| [movieclip_adaptive_curve_geometry](../tests/movieclip_adaptive_curve_geometry.cpp) | Managed getBounds/getRect |
| [movieclip_adaptive_curve_hairline](../tests/movieclip_adaptive_curve_hairline.cpp) | Hairline |
| [movieclip_adaptive_curve_mixed](../tests/movieclip_adaptive_curve_mixed.cpp) | Смешанная заливка |
| [movieclip_adaptive_curve_dimensions](../tests/movieclip_adaptive_curve_dimensions.cpp) | Размеры MovieClip |
| [movieclip_adaptive_curve_hitbox](../tests/movieclip_adaptive_curve_hitbox.cpp) | Bounding hitTest |
| [movieclip_adaptive_curve_quality](../tests/movieclip_adaptive_curve_quality.cpp) | Качество и виды paint |
| [movieclip_adaptive_curve_transfer](../tests/movieclip_adaptive_curve_transfer.cpp) | Binary/channel после teardown |
| [movieclip_adaptive_curve_threads](../tests/movieclip_adaptive_curve_threads.cpp) | Независимые потоки и GC |

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_path_stroke_bounds_style` | function | [src/capi/path_hit_c.cpp:529](../src/capi/path_hit_c.cpp#L529) |
| `FVM_UNSUPPORTED` | enum constant | [include/flashvm/flashvm.h:46](../include/flashvm/flashvm.h#L46) |
<!-- source-links:end -->
