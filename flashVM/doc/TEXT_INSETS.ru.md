# Ограниченные поля абзаца

v0.885 применяет TextFormat `leftMargin`, `rightMargin`, `blockIndent` к
однородному native-стилю TextField и TextFormat.getTextExtent. Значения — конечные
целые числа [0,4096] в native-пикселях; null/undefined сохраняют прежнее значение.
Отрицательные, дробные, NaN и бесконечные дают INVALID_ARGUMENT без изменения поля.
Отступ только первой строки `indent`, bullets и tabStops пока не поддержаны.
Это ограниченная реализация однородного стиля, не полное соответствие Flash.

Левое начало равно leftMargin + blockIndent. Перенос многострочного текста
вычитает все три значения из доступной ширины; минимум существующей внутренней
ширины обеспечивает продвижение даже слишком широкого кластера. Выравнивания
left/center/right/justify используют эту область. Без переноса выравнивание
использует ту же область. Глифы и подчёркивание получают одинаковое смещение.
`_x`/`_y` отделены от локальных контуров. textWidth и extent.width измеряют advance
текста без полей; autoSize без переноса и extent.textFieldWidth включают поля и
внутренний gutter. При переносе автоматическая ширина фиксирована, высота
соответствует новым строкам. Пустой текст имеет естественную высоту пустой строки,
а автоматическую ширину — сумму полей и gutter.

Текущий и будущий форматы независимы. Полное присваивание использует будущий
стиль; замена сохраняет текущий однородный стиль, кроме первого ввода в пустое
поле. Все проверки и подготовка геометрии завершаются до публикации. Ограничения
шрифтов, раскладки, UTF-8/UTF-16 и владения контекстом сохраняются.

[FVM_TEXT_STYLE_HAS_INSETS](../src/standard/text_constants.h#L20) добавляет три необязательных числа после необязательного
letterSpacing в native-снимке. Старые записи получают нули; версия wire и
публичные C-структуры не меняются. Текущие/будущие значения передаются отдельно.
Старые readers отклоняют новый флаг вместо потери видимого форматирования.
Восстановление проверяет целые границы и соответствие флага до регистрации шрифта
и присоединения ресурса. Кэши раскладки перестраиваются, не разделяются.

[fvm_object_set](../include/flashvm/value.h#L74) сохраняет свойства TextFormat; [fvm_object_call](../include/flashvm/value.h#L126) применяет
setTextFormat/setNewTextFormat. Заимствованные управляемые объекты принадлежат
одному контексту. getTextFormat/getNewTextFormat возвращают управляемые снимки;
для сохранения через GC нужны обычные корни/pins. Вызовы выполняются потоком
владельца контекста и могут выделять память. OUT_OF_MEMORY сохраняет текст,
стиль и геометрию поля. [fvm_textfield_path](../include/flashvm/standard_text.h#L48) возвращает независимый локальный путь,
который освобождается [fvm_path_destroy](../include/flashvm/path.h#L52) и может пережить контекст. Нового C API
нет. Для конкуренции контексты требуют отдельных владельцев.

[Справочник Adobe](https://open-flash.github.io/mirrors/as2-language-reference/TextFormat.html)
описывает поля и отступ всего блока. Сравнения с оригинальным Player, полная
загрузка SWF-шрифтов, устройства/платформы и все девять стадий остаются открытыми.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_object_call` | function | [include/flashvm/value.h:126](../include/flashvm/value.h#L126) |
| `fvm_object_set` | function | [include/flashvm/value.h:74](../include/flashvm/value.h#L74) |
| `fvm_path_destroy` | function | [include/flashvm/path.h:52](../include/flashvm/path.h#L52) |
| `FVM_TEXT_STYLE_HAS_INSETS` | macro / define | [src/standard/text_constants.h:20](../src/standard/text_constants.h#L20) |
| `fvm_textfield_path` | function | [include/flashvm/standard_text.h:48](../include/flashvm/standard_text.h#L48) |
<!-- source-links:end -->
