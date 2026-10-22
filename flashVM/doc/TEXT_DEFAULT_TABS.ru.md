# Метрика стандартной табуляции

Функция [fvm_font_average_width](../include/flashvm/font.h#L65) читает положительный OS/2 xAvgCharWidth и масштабирует через FreeType до нехинтованных пикселей 26.6. Пробел, максимальная ширина и среднее по произвольной строке не подставляются. Живой шрифт заимствуется на время вызова; доступ к изменяемому face сериализует вызывающий. Выделений памяти, VM и корней GC нет. Обязательный out обнуляется при ошибке. Неверные указатели дают INVALID_ARGUMENT, отсутствующая/неположительная метрика — UNSUPPORTED; NO_FONTS с корректным out возвращает UNSUPPORTED. Структуры и прежний ABI не меняются.

В тестовом шрифте независимо записаны 547 единиц при 1000 units/em: размер 40 даёт 21.875, размер 20 — 10.9375 пикселя. Пока это только метрика, не поддержка стандартных табуляций.

Источник: [спецификация OS/2 OpenType](https://learn.microsoft.com/en-us/typography/opentype/spec/os2#xavgcharwidth). Эталонная приёмка Flash и устройств остаётся открытой.

v0.916: [fvm_text_layout_create_periodic](../include/flashvm/text_layout.h#L155) принимает конечный положительный интервал до 1 000 000 пикселей. U+0009 переходит к положительному кратному строго правее пера с учётом indent и интервала букв. Позиции начинаются заново на каждой визуальной строке; таблица на 64 элемента и её выделение не нужны. Сохраняются пределы глифов/работы/геометрии, пустой контур табуляции, переносы, владение и статусы custom API. Нужны LTR с уровнями bidi 0 и выравнивание не JUSTIFY. Прежние API по-прежнему отклоняют табуляцию без таблицы. NO_FONTS проверяет интервал/флаги/spacing/indent, затем возвращает UNSUPPORTED без чтения font/text/options. NULL out и неверный интервал дают INVALID_ARGUMENT; выход обнуляется.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_font_average_width` | function | [include/flashvm/font.h:65](../include/flashvm/font.h#L65) |
| `fvm_text_layout_create_periodic` | function | [include/flashvm/text_layout.h:155](../include/flashvm/text_layout.h#L155) |
<!-- source-links:end -->
