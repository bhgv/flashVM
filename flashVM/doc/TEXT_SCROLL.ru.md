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


## Свойства AS2

scroll и hscroll доступны для записи; maxscroll, maxhscroll и bottomScroll — только
для чтения. Конечное числовое преобразование усекается и ограничивается диапазоном.
NaN/infinity и ошибка conversion отклоняются без изменения offsets этим setter;
явные побочные эффекты пользовательского conversion сохраняются. Раскладка
измеряется после conversion, поэтому reentry не публикует устаревшую геометрию.
Успешный query/set нормализует обе координаты. Text/style/dimension edits сохраняют
lazy layout; clamp выполняется при следующем scroll observation/export/capture.
Уведомление onScroller не создаётся. Старые callback selectors сохраняют ID;
новые добавлены в конец, upgrade заменяет только rootless старые native stubs.


## Владение при binary/channel передаче

Ненулевое состояние прокрутки добавляет [FVM_TEXT_STATE_HAS_SCROLL](../src/standard/text_constants.h#L49) и два числа
после optional position. При default scroll=1/hscroll=0 эти байты отсутствуют;
старый payload восстанавливает defaults. Заряд snapshot включает optional 26
wire bytes. Capture измеряет/clamp активные позиции; default fields сохраняют
прежний lazy capture. Restore отклоняет нечисловые/дробные/отрицательные значения,
превышение переносимых границ или фактической геометрии до публикации field.
Шрифты и текст receiver владеет независимо; font/cache/VM pointers не передаются.
Новый бит требует обновлённого receiver. Upgrade меняет только rootless старые
scroll stubs; captured/custom accessors сохраняют семантику.


## Lazy edits и наблюдательный capture

Text/format/dimension writes сохраняют прежний lazy layout admission. Запрос scroll
может временно превышать диапазон после уменьшения viewport. Scroll property/native
query и успешный path export нормализуют обе сохранённые позиции; ошибка сохраняет
их. Graph capture копирует эффективные clamped offsets без публикации в source,
поэтому последующий сбой сериализации не меняет source viewport request. Если
геометрия уменьшилась и выросла до нормализующего наблюдения, прежний запрос может
снова стать эффективным. Это явная bounded-политика bridge; полная original-player
edit/event приёмка открыта. Существующая раскладка сохраняет whitespace, которая
может занимать отдельную visual line. Hard breaks при multiline=false остаются
UNSUPPORTED.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_INVALID_ARGUMENT` | enum constant | [include/flashvm/flashvm.h:45](../include/flashvm/flashvm.h#L45) |
| `FVM_TEXT_STATE_HAS_SCROLL` | macro / define | [src/standard/text_constants.h:49](../src/standard/text_constants.h#L49) |
| `fvm_textfield_scroll_info` | function | [include/flashvm/standard_text.h:68](../include/flashvm/standard_text.h#L68) |
| `FVM_TextFieldScrollInfo` | struct | [include/flashvm/standard_text.h:44](../include/flashvm/standard_text.h#L44) |
<!-- source-links:end -->
