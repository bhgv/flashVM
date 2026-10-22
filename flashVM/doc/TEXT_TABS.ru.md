# Пользовательские позиции табуляции

v0.903 добавляет [fvm_text_layout_create_tabbed](../include/flashvm/text_layout.h#L140). Аргументы шрифта, UTF-8 с длиной,
параметров, флагов shaping, знаковых letterSpacing и отступа первой строки — как
у [fvm_text_layout_create_indented](../include/flashvm/text_layout.h#L121); дополнительно stops и count. Таблица содержит
до [FVM_TEXT_LAYOUT_MAX_TAB_STOPS](../include/flashvm/text_layout.h#L14) (64) конечных, неотрицательных, строго возрастающих
native-позиций не более 1 000 000 пикселей. NULL допустим только при count=0.
Нулевой count сохраняет старый API, включая UNSUPPORTED для U+0009. Прежние
сигнатуры и публичные структуры не меняются.

U+0009 продвигает перо до первой позиции строго правее текущей, с учётом отступа
первой строки и уже добавленных интервалов. Сам tab не получает letterSpacing.
Таблица начинается заново на каждой визуальной строке; позиции измеряются до
выравнивания от начала содержимого абзаца. Автоматические/повторяемые позиции
не выводятся. Исчерпание таблицы может вызвать перенос перед tab; если и в
начале новой строки позиции нет, результат UNSUPPORTED. Слишком широкий tab
следует прежней политике неделимого grapheme. Явный новый абзац заново применяет
отступ первой строки.

Tab сохраняет исходный байтовый кластер и синтетическую запись glyph=0 с реальным
advance и пустым контуром. Экспорт не рисует .notdef шрифта для tab. Shaping слов,
необязательные лигатуры и ограничения обязательных кластеров между tab сохраняются.
Абзацы с tab пока требуют нулевых bidi-уровней LTR и отклоняют JUSTIFY. Неиспользуемая
таблица не ограничивает остальные допустимые абзацы. Left/center/right/START
смещают целую строку. Размер исключает выравнивающее заполнение, включает исходный
отступ и сохраняет диапазоны. Подчёркивание включает реальные advances tab.

Шрифт/текст/параметры/таблица заимствуются только на вызов. [FVM_TextLayout](../src/capi/text_layout_c.cpp#L13) владеет
метаданными, глифами и контурами; освобождение [fvm_text_layout_destroy](../include/flashvm/text_layout.h#L84). Экспорт
[fvm_text_layout_path](../include/flashvm/text_layout.h#L153)/fvm_text_layout_glyph_path возвращает отдельные [FVM_Path](../src/capi/path_c.cpp#L26),
освобождаемые [fvm_path_destroy](../include/flashvm/path.h#L52) и способные пережить раскладку/шрифт. Шрифт должен
быть действующим, text может быть NULL только при нулевой длине. NULL options
выбирает неограниченную ширину, естественную высоту и AUTO/START. Границы флагов,
интервалов/отступа прежние. VM/GC не участвуют. Доступ к изменяемому шрифту
сериализует вызывающий; неизменяемая раскладка допускает независимых читателей.

Неверные таблица/указатели/count/порядок/координаты, флаги/параметры/UTF-8,
интервалы/отступ дают INVALID_ARGUMENT. Bidi/justify/исчерпание таблицы, сложность
и координатные границы — UNSUPPORTED; отказ памяти — OUT_OF_MEMORY. Ошибки обнуляют
обязательный out. NO_FONTS проверяет таблицу, флаги, интервалы/отступ, затем даёт
UNSUPPORTED без чтения текста/параметров/шрифта. Автоматический шаг tab, полные
Unicode/bidi-табуляторы, сравнение с Flash и устройства/платформы остаются открытыми.

v0.906 применяет TextFormat.tabStops из native Array того же контекста. Длина
считывается один раз (0..64), затем индексы читаются по порядку через обычный
property bridge. Позиции — целые Number в [0,1000000], строго возрастающие.
Пустые слоты и неверные числа дают INVALID_ARGUMENT, другой тип — UNSUPPORTED.
Ошибки getters/отмена работы передаются вызывающему. [GC:] массив удерживается
корнем во время getters; сохраняются только числа. Побочные действия getters
наблюдаемы и при отказе применения поля не откатываются.

Null/undefined сохраняют стиль; пустой Array очищает таблицу, оставляя default
tab неподдержанным. Полное присваивание использует будущий стиль; текущее
форматирование/замена — текущий однородный. Непустая таблица заставляет проверить
геометрию до публикации, в том числе ручного поля. getTextFormat/getNewTextFormat
возвращают новые управляемые Arrays (пустой Array при отсутствии таблицы).
Изменение исходного массива/снимка не редактирует поле. Управляемые результаты
нужно удерживать корнями через GC в потоке-владельце.

[FVM_TEXT_STYLE_HAS_TABS](../src/standard/text_constants.h#L27) добавляет ограниченный u32 count и tagged числа после
необязательного indent. Старые записи получают пустую таблицу, текущий/будущий
стили передаются отдельно, старые readers отклоняют новый флаг. Count проверяется
до выделения; восстановление проверяет границы/порядок/целые значения и
соответствие флага до импорта шрифта этого стиля. Прежнее время жизни шрифтов при
импорте графа сохраняется. Публичные C-структуры и версия wire прежние.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_Path` | struct | [src/capi/path_c.cpp:26](../src/capi/path_c.cpp#L26) |
| `fvm_path_destroy` | function | [include/flashvm/path.h:52](../include/flashvm/path.h#L52) |
| `fvm_text_layout_create_indented` | function | [include/flashvm/text_layout.h:121](../include/flashvm/text_layout.h#L121) |
| `fvm_text_layout_create_tabbed` | function | [include/flashvm/text_layout.h:140](../include/flashvm/text_layout.h#L140) |
| `fvm_text_layout_destroy` | function | [include/flashvm/text_layout.h:84](../include/flashvm/text_layout.h#L84) |
| `fvm_text_layout_glyph_path` | function | [include/flashvm/text_layout.h:168](../include/flashvm/text_layout.h#L168) |
| `FVM_TEXT_LAYOUT_MAX_TAB_STOPS` | macro / define | [include/flashvm/text_layout.h:14](../include/flashvm/text_layout.h#L14) |
| `fvm_text_layout_path` | function | [include/flashvm/text_layout.h:153](../include/flashvm/text_layout.h#L153) |
| `FVM_TEXT_STYLE_HAS_TABS` | macro / define | [src/standard/text_constants.h:27](../src/standard/text_constants.h#L27) |
| `FVM_TextLayout` | struct | [src/capi/text_layout_c.cpp:13](../src/capi/text_layout_c.cpp#L13) |
<!-- source-links:end -->
