# Аналитические границы круглых Bezier-штрихов

Текущая поддержка монотонных осевых команд: [контракт](AXIS_CURVE_BOUNDS.ru.md). Прежний общий отказ non-round кривых ниже имеет это исключение.

Внутренний [fvm_path_stroke_bounds_style](../src/capi/path_hit_c.cpp#L432) теперь принимает quadratic и cubic при round/round. Он измеряет аналитические экстремумы центральной линии каждого непостоянного сегмента и расширяет объединённую рамку на половину эффективной ширины. Для полностью круглого штриха это границы радиусной окрестности центральной линии, с обычной погрешностью double. Результат является геометрическим, а не пиксельной рамкой сглаживания или подтверждением полного совпадения с Flash.

Move лишь меняет начало контура и не создаёт штрих. Постоянные line/quadratic/cubic не добавляют область; совпадение концов кривой само по себе не делает её постоянной, если отличается control point. Close добавляет реальный замыкающий сегмент. Несколько контуров объединяются без добавления промежуточных перемещений. Пустая область возвращает нулевую рамку и has_segments=0.

## Контракт и владение

[fvm_path_stroke_bounds_style](../src/capi/path_hit_c.cpp#L432) заимствует неизменяемый путь и принимает конечную положительную ширину до 1 000 000 единиц назначения, допустимые cap/join и два обязательных принадлежащих вызывающему указателя результата. Выделений памяти, callbacks, GC или контекста нет. Ошибки сохраняют оба результата. Неизвестные selectors/недопустимые width/указатели дают [FVM_INVALID_ARGUMENT](../include/flashvm/flashvm.h#L45). Кривые с butt/square или bevel по-прежнему дают [FVM_UNSUPPORTED](../include/flashvm/flashvm.h#L46) без записи результатов. Совместная мутация пути запрещена.

Для поиска корней производной используется общий [внутренний математический модуль](../src/render/curve_bounds.hpp), извлечённый из существующего [fvm_path_bounds](../src/capi/path_c.cpp#L194). Формулы публичной функции и её прежний учёт одиночных Move сохраняются. Новый round-обход специально исключает нештриховые Move. Нулевой authored width по-прежнему превращается в одну единицу назначения в managed-обходе; positive width использует прежнюю метрику максимальной оси. Полное схлопывание положительной ширины не создаёт штрих.

## MovieClip и ограничения

Существующий bounds-обход MovieClip автоматически использует новый результат для quadratic round/round: getBounds, размеры и hitTest без shape больше не включают далёкие одиночные moveTo в границы только кривого штриха. Заливка продолжает учитывать исходную рамку пути; смешанный paint объединяет её со штрихом. getRect сохраняет прежнюю геометрию. Путь и снимки managed-обхода по-прежнему выделяют память и используют существующий GC-барьер.

Cubic доступен нативному path API; managed MovieClip намеренно отклоняет cubic-команды. AS2/SWF-семантика этим не расширяется до cubic. Point hitTest с shape и растеризация не меняются и сохраняют собственное разбиение кривых. Для butt/square/bevel кривые остаются в консервативном fallback. Точные некруглые offset-границы, неоднородный масштаб/shear, полная reference-растеризация, miter/hinting/non-scaling, Cairo и реальные устройства остаются открытыми. Публичный C89 ABI, callback ID и wire-форматы не меняются. Все девять этапов roadmap открыты.

Исполняемый C89-пример [stroke_styles](../examples/embedding/stroke_styles/README.ru.md) расширяется в этой серии отдельной round-кривой с явной заменой GC-корня результата.

## Проверенная контрольная точка v0.1358

Серия v0.1338–v0.1358 поверх опубликованного v0.1337 содержит 21 локальный коммит. Пройдены 275 выбранных нативных AGG/JIT-регрессий и отдельный существующий C89 vector_paths (всего 276), 22 теста без графики и 22 ASan/UBSan, включая окончательный C89-пример. LeakSanitizer отключён. Восемнадцать новых fixtures покрывают native quadratic/cubic, возвратные и постоянные кривые, раздельные контуры, поворот/масштаб, предельные координаты, атомарность результатов, managed quadratic и намеренный отказ cubic, mixed paint, размеры, bounding hitTest, четыре качества/четыре paint, снимки, четыре графа через binary и channel после удаления отправителя и четыре OS-потока с 96 циклами bounds/GC. Существующие нативные тесты включают JIT, таймлайн, каналы и отказы памяти; это не отдельная полная reference-приёмка новых кривых для каждой подсистемы. Проверки ссылок, примеров и форматирования пройдены. Cairo и физические платформы не проверялись; все девять этапов открыты.

| Fixture | Проверка |
| --- | --- |
| [path_round_curve_quadratic](../tests/path_round_curve_quadratic.cpp) | Квадратичные экстремумы |
| [path_round_curve_cubic](../tests/path_round_curve_cubic.cpp) | Кубические экстремумы |
| [path_round_curve_reversal](../tests/path_round_curve_reversal.cpp) | Непостоянная возвратная кривая |
| [path_round_curve_degenerate](../tests/path_round_curve_degenerate.cpp) | Постоянные сегменты |
| [path_round_curve_contours](../tests/path_round_curve_contours.cpp) | Объединение активных контуров |
| [path_round_curve_rotation](../tests/path_round_curve_rotation.cpp) | Поворот и равномерный масштаб |
| [path_round_curve_limits](../tests/path_round_curve_limits.cpp) | Пределы координат и ширины |
| [path_round_curve_outputs](../tests/path_round_curve_outputs.cpp) | Сохранение обоих результатов при ошибке |
| [movieclip_round_curve_isolated](../tests/movieclip_round_curve_isolated.cpp) | Одиночные moveTo в managed-кривой |
| [movieclip_round_curve_degenerate](../tests/movieclip_round_curve_degenerate.cpp) | Постоянные managed-кривые и getRect |
| [movieclip_round_curve_cubic](../tests/movieclip_round_curve_cubic.cpp) | Отказ managed cubic и восстановление |
| [movieclip_round_curve_mixed](../tests/movieclip_round_curve_mixed.cpp) | Смешанные заливка и штрих |
| [movieclip_round_curve_dimensions](../tests/movieclip_round_curve_dimensions.cpp) | Размеры hairline-кривой |
| [movieclip_round_curve_hitbox](../tests/movieclip_round_curve_hitbox.cpp) | Рамка hitTest без shape |
| [movieclip_round_curve_quality](../tests/movieclip_round_curve_quality.cpp) | Качество Stage и виды paint |
| [movieclip_round_curve_snapshots](../tests/movieclip_round_curve_snapshots.cpp) | Снимки стиля clear и GC |
| [movieclip_round_curve_transfer](../tests/movieclip_round_curve_transfer.cpp) | Передача binary и channel |
| [movieclip_round_curve_threads](../tests/movieclip_round_curve_threads.cpp) | Независимые рабочие потоки |

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_INVALID_ARGUMENT` | enum constant | [include/flashvm/flashvm.h:45](../include/flashvm/flashvm.h#L45) |
| `fvm_path_bounds` | function | [include/flashvm/path.h:71](../include/flashvm/path.h#L71) |
| `fvm_path_stroke_bounds_style` | function | [src/capi/path_hit_c.cpp:432](../src/capi/path_hit_c.cpp#L432) |
| `FVM_UNSUPPORTED` | enum constant | [include/flashvm/flashvm.h:46](../include/flashvm/flashvm.h#L46) |
<!-- source-links:end -->
