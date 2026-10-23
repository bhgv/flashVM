# Точные границы монотонных осевых Bezier-команд

Проверку допустимого сокращения выполняет [axis_curve_segment](../src/capi/path_hit_c.cpp#L236).

Внутренний расчёт [fvm_path_stroke_bounds_style](../src/capi/path_hit_c.cpp#L432) принимает quadratic/cubic для всех поддержанных cap/join, когда управляющие точки упорядочены от начала к концу на одной горизонтальной или вертикальной оси. Такая кривая описывает точно отрезок: производная по изменяющейся координате имеет один знак. Проверка использует точные сравнения double без epsilon и не создаёт приближённую полилинию. Контроль в начале/конце допустим, в том числе при нулевой производной на торце. Полностью постоянная команда не создаёт области.

Участок обрабатывается существующим расчётом конечного тела, торцов и соединений. Round/none/square и round/bevel сохраняют свои прежние значения; соединения с соседними отрезками и Close используют те же нормали. Move без штриха не расширяет рамку. Возвратные, неосевые или неупорядоченные controls не принимаются новым сокращением, даже если более сложный анализ мог бы доказать монотонность. Для non-round они возвращают [FVM_UNSUPPORTED](../include/flashvm/flashvm.h#L46), сохраняя оба результата; managed-обход выбирает прежний консервативный fallback. Round/round продолжает поддерживать общие аналитические кривые.

## Контракт и интеграция

[fvm_path_stroke_bounds_style](../src/capi/path_hit_c.cpp#L432) заимствует неизменяемый путь, положительную конечную ширину до 1 000 000 единиц назначения, selectors и два обязательных принадлежащих вызывающему результата. Выделений, контекста, callbacks и GC нет. Ошибки аргументов и unsupported сохраняют оба результата. Путь нельзя одновременно изменять. Лимиты заданы в [path_geometry_constants.h](../src/capi/path_geometry_constants.h). [Внутренний обход](../src/capi/path_hit_c.cpp) публикует результаты после всего пути.

Managed quadratic автоматически использует точную рамку в getBounds, размерах MovieClip и hitTest без shape. Смешанная заливка сохраняет объединение с исходной рамкой пути; getRect не меняется. Hairline имеет эффективную ширину 1, положительная ширина сохраняет метрику максимальной оси. Cubic остаётся нативной возможностью path API; MovieClip по-прежнему отклоняет cubic. Растеризация и shape-hit разбиение кривых не меняются. Публичный C89 ABI, wire-форматы и callback ID сохраняются.

Произвольные повороты осевого пути могут вывести его из поддержанного класса; точные non-round кривые, неравномерный масштаб/shear, miter/hinting/non-scaling, полная Flash-reference-приёмка, Cairo и физические устройства остаются открытыми. Все девять этапов roadmap открыты. Исполняемый [C89-пример](../examples/embedding/stroke_styles/README.ru.md) демонстрирует реальный curveTo(8,8,14,8), none/bevel и игнорируемый одиночный moveTo.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_path_stroke_bounds_style` | function | [src/capi/path_hit_c.cpp:432](../src/capi/path_hit_c.cpp#L432) |
| `FVM_UNSUPPORTED` | enum constant | [include/flashvm/flashvm.h:46](../include/flashvm/flashvm.h#L46) |
<!-- source-links:end -->
