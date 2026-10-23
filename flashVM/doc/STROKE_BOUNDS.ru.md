# Границы прямолинейных штрихов с выбранными стилями

Текущая поддержка монотонных осевых команд: [контракт](AXIS_CURVE_BOUNDS.ru.md). Прежний общий отказ non-round кривых ниже имеет это исключение.

Текущий расчёт round/round кривых: [аналитические границы](ROUND_CURVE_BOUNDS.ru.md). Прежний общий fallback ниже является историческим для полностью круглого стиля.

Текущая поправка round/hairline: [контракт](ROUND_STROKE_BOUNDS.ru.md). Прежняя отдельная round-only политика ниже является исторической.

Поддержанные cap/join параметры script lineStyle связаны в [SCRIPT_STROKE_STYLES.ru.md](SCRIPT_STROKE_STYLES.ru.md). Прежние файловые/нативные checkpoints ниже не подтверждают полную расширенную AS2-семантику.

MovieClip.getBounds и потребители общего обхода геометрии учитывают торцы и
соединения прямолинейных путей с некруглым торцом или bevel-соединением. Butt
больше не получает продолжение круглого торца; диагональный square включает
действительные углы квадрата. Размеры корневого клипа и точечный hitTest без
shape используют обновлённые границы. getRect по-прежнему измеряет сам путь без
расширения штрихом. Сплошные, линейные/фокальные градиентные и bitmap-краски
используют общую геометрию; альфа и пиксели bitmap не читаются для расчёта bounds.

## Контракт внутренней геометрии

[fvm_path_stroke_bounds_style](../src/capi/path_hit_c.cpp#L455) заимствует неизменяемый путь, положительную конечную
ширину до 1 000 000 единиц назначения, поддержанные selectors и два обязательных
указателя на результат вызывающей стороны. Сохраняются ограничения координат
пути. Временная геометрия не выделяется, контекст и графический backend не нужны;
конкурентное изменение пути недопустимо. Bounds и has_segments записываются
вместе только после успешного обхода. Неверные аргументы дают [FVM_INVALID_ARGUMENT](../include/flashvm/flashvm.h#L45);
квадратичная/кубическая команда — [FVM_UNSUPPORTED](../include/flashvm/flashvm.h#L46) без изменения обоих результатов.
Пустой путь или только нулевые рёбра успешно дают нулевые bounds и отсутствие
отрезков.

Обработка контуров и касательных общая с [fvm_path_hit_test_style](../src/capi/path_hit_c.cpp#L373). Объединяются
конечные прямоугольники отрезков, экстремумы открытых торцов и внешние round-сектора.
Только направления осей внутри сектора могут добавить экстремум помимо его концов.
Вершины bevel уже принадлежат телам соседних отрезков. Замкнутый контур соединяет
последнюю и первую касательные; Move завершает торцы отдельного контура.
NoClose сохраняет открытость при совпадающих концах. Одиночный Move не расширяет
границы штриха.

Для поддержанных прямых это геометрические границы с точностью арифметики double,
а не прямоугольник пикселей AA. Публичный C89 ABI, [fvm_path_bounds](../include/flashvm/path.h#L71), отрисовка
путей и point-hit не меняются. Новые поля сериализации, callback и resource ID
не требуются.

## Управляемый обход и явный fallback

Общая stroke_selectors проверяет собственные lineCap/lineJoin без getters.
Отсутствующие поля сохраняют round. Новый расчёт выбирается для некруглого торца
или bevel; прежние round-only графы сохраняют прежние bounds, включая старую
политику hairline. Выбранный hairline использует одну единицу назначения.
Полностью схлопнутый выбранный штрих не даёт границ; заливка той же краски
сохраняет свою геометрию. Если у paint включены заливка и штрих, объединяются
границы пути заливки и выбранной линии. Пустой штрих не стирает другие paint/children.

Для кривых выбран консервативный fallback вместо потенциально недостаточных
границ хорд. Butt/round используют прежнее расширение осей на половину ширины;
square — на половину ширины, умноженную на sqrt(2), чтобы включить углы торца.
Это не точная оболочка кривой. getRect не меняется. Bounds и hit сохраняют
прежний максимальный масштаб осей, рендеринг — масштаб X. Метрики Flash для
неравномерного масштаба/shear и правила преобразования толщины в целевую систему
координат требуют отдельной приёмки.

Существующий нативный барьер и копии рёбер геометрии/display сохраняют граф при
callbacks и GC. Отказы памяти управляемого построения возвращаются через границу
нативного вызова; сам прямолинейный обход память не выделяет. Binary/channel,
замена персонажа и seek проверяют текущий граф после удаления отправителя или GC.
Изолированные рабочие потоки владеют собственным состоянием запроса.

## Оставшаяся приёмка

Открыты точные оболочки кривых, прежнее расхождение hairline для round,
неравномерные/shear метрики, miter/асимметричные торцы, hinting и non-scaling,
полная независимая Flash-приёмка, Cairo и реальные устройства/платформы.
Серия не завершает SWF <= 8 и ни один из девяти этапов плана. Примеры сохраняют
публичные round API; выбранные файловые стили проверяются регрессионными fixtures.

## Указатель fixtures

| Fixture | Проверка |
| --- | --- |
| [path_style_bounds_caps](../tests/path_style_bounds_caps.cpp) | Горизонтальные/вертикальные торцы |
| [path_style_bounds_diagonal](../tests/path_style_bounds_diagonal.cpp) | Независимые уравнения 3-4-5, четыре направления и пять ширин |
| [path_style_bounds_turns](../tests/path_style_bounds_turns.cpp) | Экстремумы round-секторов, bevel и короткие рёбра |
| [path_style_bounds_contours](../tests/path_style_bounds_contours.cpp) | Замыкание, несколько контуров и пустые штрихи |
| [path_style_bounds_errors](../tests/path_style_bounds_errors.cpp) | Атомарное отклонение аргументов и кривых |
| [swf_stroke_bounds](../tests/swf_stroke_bounds.cpp) | 24 сочетания стилей, getRect и GC |
| [swf_stroke_bounds_widths](../tests/swf_stroke_bounds_widths.cpp) | 120 сочетаний ширины и стилей с hairline |
| [swf_stroke_bounds_curves](../tests/swf_stroke_bounds_curves.cpp) | Двенадцать консервативных оболочек кривых |
| [swf_stroke_bounds_metadata](../tests/swf_stroke_bounds_metadata.cpp) | Ошибки восстановленных selectors/ширины и round по умолчанию |
| [swf_stroke_bounds_transforms](../tests/swf_stroke_bounds_transforms.cpp) | 96 равномерных преобразований и 24 схлопывания |
| [swf_stroke_bounds_mixed](../tests/swf_stroke_bounds_mixed.cpp) | Шестнадцать графов штриха/заливки/Move/вырождения |
| [swf_stroke_bounds_hitbox](../tests/swf_stroke_bounds_hitbox.cpp) | 24 проверки чтения/записи размеров корня и попаданий без shape |
| [swf_stroke_bounds_quality](../tests/swf_stroke_bounds_quality.cpp) | 96 проверок независимо от качества Stage |
| [swf_stroke_bounds_transfer](../tests/swf_stroke_bounds_transfer.cpp) | 24 графа через binary/канал после удаления отправителя |
| [swf_stroke_bounds_timeline](../tests/swf_stroke_bounds_timeline.cpp) | Замена персонажа и 16 чередующихся seek |
| [swf_stroke_bounds_threads](../tests/swf_stroke_bounds_threads.cpp) | Четыре OS-потока, 96 циклов bounds/GC |
| [swf_stroke_bounds_allocation](../tests/swf_stroke_bounds_allocation.cpp) | 24 серии отказов C++-памяти getBounds |


## Контрольная точка v0.1303

Границы выбранных штрихов v0.1285–v0.1303: прямые пути с некруглым торцом или bevel измеряются по телам отрезков, углам торцов и экстремумам внешних round-секторов вместо общего расширения рамки как для round. Результат общий для getBounds, размеров корня и hitTest без shape; getRect остаётся без толщины. Пустой штрих и одиночный Move не создают границ, смешанная заливка/линия сохраняет объединение. Внутренний обход без выделений атомарно отклоняет кривые; управляемые кривые используют явный консервативный fallback, расширенный для square в sqrt(2) раз. Семнадцать fixtures проверяют осевые и независимые диагональные уравнения, внешние сектора и короткие рёбра, замыкание, пустую геометрию, ошибки, 24 сочетания SWF-стилей, 120 ширин, 12 оболочек кривых, метаданные, 96 равномерных преобразований и 24 полных схлопывания, 16 смешанных/вырожденных графов, чтение/запись размеров корня и hit boxes, 96 качеств Stage, 24 графа через binary/канал после удаления отправителя, замену персонажа и 16 seek, четыре потока с 96 циклами bounds/GC и 24 серии отказов C++-памяти. Пройдены 228 выбранных нативных регрессий, все 17 fixtures в SDK-free интерпретаторе без графики, 16 ASan/UBSan плюс allocation с явным --no-faults и проверки документации/примеров. Реальные отказы проверены нативно; sanitizer unwinding ограничен, LeakSanitizer отключён. Публичный C89 ABI, wire-форматы и прежняя политика round-only bounds сохраняются. Точные границы кривых, расхождение round hairline, неравномерные/shear метрики, неподдержанные опции LINESTYLE2, полная независимая Flash-приёмка, Cairo и реальные платформы открыты. Все девять этапов открыты. [Контракт](STROKE_BOUNDS.ru.md).

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_INVALID_ARGUMENT` | enum constant | [include/flashvm/flashvm.h:45](../include/flashvm/flashvm.h#L45) |
| `fvm_path_bounds` | function | [include/flashvm/path.h:71](../include/flashvm/path.h#L71) |
| `fvm_path_hit_test_style` | function | [src/capi/path_hit_c.cpp:373](../src/capi/path_hit_c.cpp#L373) |
| `fvm_path_stroke_bounds_style` | function | [src/capi/path_hit_c.cpp:455](../src/capi/path_hit_c.cpp#L455) |
| `FVM_UNSUPPORTED` | enum constant | [include/flashvm/flashvm.h:46](../include/flashvm/flashvm.h#L46) |
<!-- source-links:end -->
