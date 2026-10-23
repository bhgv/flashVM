# Прокрутка одностильного TextField

Добавляется ограниченная прокрутка host-created TextField.
[Справочник Adobe AS2](https://open-flash.github.io/mirrors/as2-language-reference/TextField.html)
задаёт вертикальный индекс visual line от единицы и горизонтальные пиксели от нуля.
Используется существующая одностильная раскладка без oracle оригинального Flash.
HTML/смешанные шрифты, onScroller/listeners, ввод, текст display list SWF и приёмка
на физических платформах остаются открытыми. Все девять стадий плана открыты.

[fvm_textfield_scroll_info](../include/flashvm/standard_text.h#L68) копирует геометрию на потоке владельца context. Задайте
точный struct_size [FVM_TextFieldScrollInfo](../include/flashvm/standard_text.h#L44) и сохраните достижимость объекта.
Возможна подготовка cache layout, но нет script/callback/device. Неверный descriptor
даёт [FVM_INVALID_ARGUMENT](../include/flashvm/flashvm.h#L45); статусы layout/font/extent/выделения передаются.
Ошибка сохраняет выход и scroll, но успешно построенный cache может остаться.
Изменения field/context и GC сериализуются; результат не удерживает pointer/root.

Внутренняя область вычитает суммарный gutter четыре пикселя и ограничена нулём.
Шаг baseline равен font ascent + descent + leading. maxscroll использует количество
полностью помещающихся line bodies с минимумом одной строки даже для нулевого
viewport; bottom_scroll следует той же политике. maxhscroll — округлённое вверх
положительное переполнение логической ширины с paragraph insets. Это явные
ограниченные правила, не полная приёмка Flash rounding/version. Контуры не обрезаны:
хост использует возвращённый clip rectangle и один раз применяет _x/_y. Intrinsic
text metrics не меняются от прокрутки.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_INVALID_ARGUMENT` | enum constant | [include/flashvm/flashvm.h:45](../include/flashvm/flashvm.h#L45) |
| `fvm_textfield_scroll_info` | function | [include/flashvm/standard_text.h:68](../include/flashvm/standard_text.h#L68) |
| `FVM_TextFieldScrollInfo` | struct | [include/flashvm/standard_text.h:44](../include/flashvm/standard_text.h#L44) |
<!-- source-links:end -->
