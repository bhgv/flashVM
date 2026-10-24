# Точные границы монотонных осевых Bezier-команд

Текущее расширение: [адаптивные векторы](ADAPTIVE_CURVE_BOUNDS.ru.md). Прежнее требование абсолютной сетки ниже историческое.

Текущее расширение: [линейные кривые](LINEAR_CURVE_BOUNDS.ru.md). Прежние ограничения упорядоченности ниже исторические; helper переименован в linear_curve_segment.

Проверку допустимого сокращения выполняет [linear_curve_segment](../src/capi/path_hit_c.cpp#L317).

Внутренний расчёт [fvm_path_stroke_bounds_style](../src/capi/path_hit_c.cpp#L564) принимает quadratic/cubic для всех поддержанных cap/join, когда управляющие точки упорядочены от начала к концу на одной горизонтальной или вертикальной оси. Такая кривая описывает точно отрезок: производная по изменяющейся координате имеет один знак. Проверка использует точные сравнения double без epsilon и не создаёт приближённую полилинию. Контроль в начале/конце допустим, в том числе при нулевой производной на торце. Полностью постоянная команда не создаёт области.

Участок обрабатывается существующим расчётом конечного тела, торцов и соединений. Round/none/square и round/bevel сохраняют свои прежние значения; соединения с соседними отрезками и Close используют те же нормали. Move без штриха не расширяет рамку. Возвратные, неосевые или неупорядоченные controls не принимаются новым сокращением, даже если более сложный анализ мог бы доказать монотонность. Для non-round они возвращают [FVM_UNSUPPORTED](../include/flashvm/flashvm.h#L46), сохраняя оба результата; managed-обход выбирает прежний консервативный fallback. Round/round продолжает поддерживать общие аналитические кривые.

## Контракт и интеграция

[fvm_path_stroke_bounds_style](../src/capi/path_hit_c.cpp#L564) заимствует неизменяемый путь, положительную конечную ширину до 1 000 000 единиц назначения, selectors и два обязательных принадлежащих вызывающему результата. Выделений, контекста, callbacks и GC нет. Ошибки аргументов и unsupported сохраняют оба результата. Путь нельзя одновременно изменять. Лимиты заданы в [path_geometry_constants.h](../src/capi/path_geometry_constants.h). [Внутренний обход](../src/capi/path_hit_c.cpp) публикует результаты после всего пути.

Managed quadratic автоматически использует точную рамку в getBounds, размерах MovieClip и hitTest без shape. Смешанная заливка сохраняет объединение с исходной рамкой пути; getRect не меняется. Hairline имеет эффективную ширину 1, положительная ширина сохраняет метрику максимальной оси. Cubic остаётся нативной возможностью path API; MovieClip по-прежнему отклоняет cubic. Растеризация и shape-hit разбиение кривых не меняются. Публичный C89 ABI, wire-форматы и callback ID сохраняются.

Произвольные повороты осевого пути могут вывести его из поддержанного класса; точные non-round кривые, неравномерный масштаб/shear, miter/hinting/non-scaling, полная Flash-reference-приёмка, Cairo и физические устройства остаются открытыми. Все девять этапов roadmap открыты. Исполняемый [C89-пример](../examples/embedding/stroke_styles/README.ru.md) демонстрирует реальный curveTo(8,8,14,8), none/bevel и игнорируемый одиночный moveTo.

## Проверенная контрольная точка v0.1379

Серия v0.1359–v0.1379 поверх опубликованного master v0.1358 содержит 21 локальный коммит. Пройдены 289 выбранных нативных AGG/JIT-тестов, 43 теста интерпретатора без графики и 43 ASan/UBSan, включая обновлённый C89-пример и существующий vector_paths. LeakSanitizer отключён. Восемнадцать новых fixtures проверяют два порядка кривых, шесть стилей, обе оси/направления, торцевые controls, постоянные команды, контуры, joins/Close, четыре неподдержанных случая с сохранением обоих результатов, пять ширин, managed getBounds/getRect/hairline/mixed/dimensions/hitTest, 96 quality/paint/style сочетаний, 24 графа через binary и канал после удаления отправителя, четыре OS-потока с 96 циклами bounds/GC. Существующий нативный набор включает JIT, таймлайн и отказы памяти; это регрессии, а не полная новая reference-приёмка каждой подсистемы. Пройдены source/example/diff checks. Cairo и реальные платформы не проверялись; все девять этапов остаются открытыми.

| Fixture | Проверка |
| --- | --- |
| [path_axis_curve_quadratic](../tests/path_axis_curve_quadratic.cpp) | Quadratic: все торцы и соединения |
| [path_axis_curve_cubic](../tests/path_axis_curve_cubic.cpp) | Cubic: все торцы и соединения |
| [path_axis_curve_vertical](../tests/path_axis_curve_vertical.cpp) | Вертикальная ось |
| [path_axis_curve_reverse](../tests/path_axis_curve_reverse.cpp) | Обратное направление |
| [path_axis_curve_endpoint_controls](../tests/path_axis_curve_endpoint_controls.cpp) | Нулевые производные торцов |
| [path_axis_curve_constant](../tests/path_axis_curve_constant.cpp) | Постоянные команды |
| [path_axis_curve_contours](../tests/path_axis_curve_contours.cpp) | Изолированные и постоянные контуры |
| [path_axis_curve_joins](../tests/path_axis_curve_joins.cpp) | Соединения и замыкание |
| [path_axis_curve_errors](../tests/path_axis_curve_errors.cpp) | Сохранность результатов при отказе |
| [path_axis_curve_widths](../tests/path_axis_curve_widths.cpp) | Дробная и предельная ширина |
| [movieclip_axis_curve_geometry](../tests/movieclip_axis_curve_geometry.cpp) | Managed bounds и getRect |
| [movieclip_axis_curve_hairline](../tests/movieclip_axis_curve_hairline.cpp) | Managed hairline |
| [movieclip_axis_curve_mixed](../tests/movieclip_axis_curve_mixed.cpp) | Смешанная заливка и штрих |
| [movieclip_axis_curve_dimensions](../tests/movieclip_axis_curve_dimensions.cpp) | Размеры и равномерный масштаб |
| [movieclip_axis_curve_hitbox](../tests/movieclip_axis_curve_hitbox.cpp) | Bounding hitTest |
| [movieclip_axis_curve_quality](../tests/movieclip_axis_curve_quality.cpp) | Виды paint и качество Stage |
| [movieclip_axis_curve_transfer](../tests/movieclip_axis_curve_transfer.cpp) | Передача графов binary/channel |
| [movieclip_axis_curve_threads](../tests/movieclip_axis_curve_threads.cpp) | Независимые OS-потоки |

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_path_stroke_bounds_style` | function | [src/capi/path_hit_c.cpp:564](../src/capi/path_hit_c.cpp#L564) |
| `FVM_UNSUPPORTED` | enum constant | [include/flashvm/flashvm.h:46](../include/flashvm/flashvm.h#L46) |
<!-- source-links:end -->
