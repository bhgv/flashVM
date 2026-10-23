# Заливки SWF 8 DefineShape4

Ограниченный декодер [fvm_swf_read_shape](../src/swf/shape.cpp#L427) принимает DefineShape4 (тег 83) без
обводок. Загрузчик сцены разрешает тег начиная с SWF 8. Проверки резервных битов
и ограничение в восемь точек для DefineShape/2/3 сохранены.

Поддержаны 1–15 упорядоченных RGBA-точек, pad/reflect/repeat, RGB/linearRGB,
линейные, радиальные и фокусные радиальные градиенты. Фокус читается как знаковый
little-endian FIXED8 без преобразования через скриптовое число. Поддержан только
строгий интервал (-1, 1); границы и внешние значения возвращают [FVM_UNSUPPORTED](../include/flashvm/flashvm.h#L46).
Резервные режимы, повреждённые и усечённые записи возвращают [FVM_INVALID_ARGUMENT](../include/flashvm/flashvm.h#L45).
Совпадающие позиции точек допустимы.

ShapeBounds и EdgeBounds проверяются на порядок и пределы координат. Для
размещения заливок сохраняется ShapeBounds. Резервные биты заголовка SWF 8,
включая добавленный в SWF 10 winding-rule, должны быть нулевыми. Два флага-подсказки
обводок принимаются без влияния на заливку. Любой непустой банк LINESTYLE2,
включая добавленный StateNewStyles, возвращает [FVM_UNSUPPORTED](../include/flashvm/flashvm.h#L46). Концы, соединения,
заливки и масштабирование обводок требуют отдельной реализации.

Сплошные и bitmap-заливки используют существующие пути и проверку словаря.
fvm_swf_shape_graph создаёт собственный управляемый дескриптор градиента с
необязательными скалярными свойствами spread/interpolation/focal. Указатели
бэкенда и исходные байты не сохраняются. Нативный барьер защищает построение,
рёбра графа удерживают точки и команды. Ошибка декодирования сохраняет прежний
результат вызывающего кода.

Действуют существующие ограничения семплера по округлению, трансформациям,
качеству и памяти. Полная поддержка DefineShape4 и SWF 8 пока не заявляется.
Проверки устройств, сравнение с Flash и все этапы плана остаются открытыми.

Первоисточник: Adobe SWF specification, Shapes и Gradients, страницы 132–136:
https://open-flash.github.io/mirrors/swf-spec-19.pdf

## Проверки

`tests/swf_shape4.cpp` строит байты тега и проверяет метаданные и полные поверхности
для 18 сочетаний типа, spread и интерполяции, сравнивая с публичным семплером.
Независимые аналитические проверки семплера находятся в `gradient_focal_edges.c`
и `gradient_linear_rgb.c`.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_INVALID_ARGUMENT` | enum constant | [include/flashvm/flashvm.h:45](../include/flashvm/flashvm.h#L45) |
| `fvm_swf_read_shape` | function | [src/swf/shape.cpp:427](../src/swf/shape.cpp#L427) |
| `FVM_UNSUPPORTED` | enum constant | [include/flashvm/flashvm.h:46](../include/flashvm/flashvm.h#L46) |
<!-- source-links:end -->
