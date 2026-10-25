# Ограниченное выравнивание native-абзацев по ширине

v0.877 добавляет [FVM_TEXT_ALIGN_JUSTIFY](../include/flashvm/text_layout.h#L28) в [FVM_TextLayoutOptions](../include/flashvm/text_layout.h#L33) для прежних
[fvm_text_layout_create](../include/flashvm/text_layout.h#L94), [fvm_text_layout_create_configured](../include/flashvm/text_layout.h#L106) и
[fvm_text_layout_create_spaced](../include/flashvm/text_layout.h#L122). Layout публичной структуры и прежние значения enum
не меняются. Вызовы заимствуют шрифт, counted UTF-8 и options; возвращают независимый
[FVM_TextLayout](../src/capi/text_layout_c.cpp#L15), освобождаемый через [fvm_text_layout_destroy](../include/flashvm/text_layout.h#L99). Экспортированные
контуры отдельно принадлежат вызывающему и переживают font/layout. Общий изменяемый
шрифт требует синхронизации вызывающего; неизменяемый layout читается независимо
в потоках. VM, GC и устройства не используются.

Пока допустимы LTR-абзацы с нулевыми bidi levels. Каждому ASCII U+0020 нужен
отдельный однозначный глиф. Лигатуры слов и порядок комбинируемых глифов сохраняются.
Конечные ASCII-пробелы получают нулевые advances, начальные сохраняют прежние,
а внутренние пробелы непоследних перенесённых строк поровну делят свободную ширину.
Последние строки, жёсткие концы абзацев и строки без внутренних пробелов остаются
естественной ширины. Слишком широкий неделимый кластер не сжимается. Байтовые
и glyph-записи пробелов остаются. Пробные переносы используют то же измерение
без конечных пробелов. Другие выравнивания сохраняют прежнюю семантику.

Неизвестный enum options даёт INVALID_ARGUMENT с очищением выхода в профиле со
шрифтами. Неподдержанные bidi/пробельные кластеры и превышение ограничений дают
UNSUPPORTED, ошибка памяти — OUT_OF_MEMORY. NO_FONTS сохраняет прежнюю проверку
flags/spacing и возвращает UNSUPPORTED без чтения options. Прежние границы scalars,
байтов, shaping-work и координат действуют. Это не полное выравнивание Arabic/CJK/
Unicode и не подтверждение совместимости Flash.

Исходный CC0-шрифт при 40px даёт A/V по 24px и пробел 12px без кернинга. При ширине
90 строка A V A V переносится на две: первая заполняет 90, V стоит в x=66,
конечный пробел имеет нулевой advance; последняя строка сохраняет ширину 60.
Исходные кластеры не теряются. Все стадии roadmap и проверки устройств открыты.

[Справочник Adobe AS2 TextFormat](https://open-flash.github.io/mirrors/as2-language-reference/TextFormat.html)
определяет justify как выравнивание Flash8; точное сравнение с Flash отдельно.

## Политика AS2-поля и границы пробелов — v0.878

TextFormat align принимает justify в незагруженном host-контексте и загруженном
SWF8; старые загруженные фильмы дают UNSUPPORTED при применении/измерении формата.
Многострочный TextField с wordWrap использует ту же политику, включая autoSize
и advances подчёркивания. Без переносов поле сохраняет левое размещение.
Ручной justified-режим тоже проверяет layout до публикации текста/политик/формата;
неподдержанное bidi-изменение с переносами сохраняет прежнее поле. Снимки текущего
и будущего формата возвращают justify. Граф использует отдельный style-бит,
не прежние alignment/kerning; новые числовые payload-поля не добавляются.
Старые записи сохраняют выравнивание, старый reader отвергает новый флаг.

Проверяются повторные/начальные/конечные пробелы, жёсткие концы, CRLF, пустой
последний абзац и лигатуры слов. Естественная ширина прямо суммирует сохранённые
advances, избегая потери точности при дробном трекинге строки из пробелов.
Неоднозначный пробел с combining и bidi возвращают отказ с очищенным выходом.

v0.879 проверяет текущий justify и будущий right/bold через binary/channel, уничтожение отправителя, GC получателя и последующие изменения. Четыре лица с +1 tracking дают один и тот же 90px участок подчёркивания; строка из пробелов даёт пустой контур. Выбранные шрифты, текущая/будущая политика и авто-высота независимы.

v0.880 проверяет отказ реально загруженных SWF6/7 и приём SWF8, включая native-восстановление того же состояния. Конфликт low alignment, reserved и unknown flags отвергается до подключения ресурса. C-проверка передаёт неизвестные enum bytes и явный RTL; NO_FONTS сохраняет документированный режим непрозрачных options.

v0.881 добавляет семь переборов C++ allocation в обычной сборке: native layout и текущий формат, запись/замена текста, autoSize, ширина и политика переносов. Ошибки сохраняют текст, выравнивание, размеры и кэш метрик с балансом GC/native scopes. После отказа конструктора шрифт пригоден для повторного вызова. Отказы C allocator не инжектируются.

v0.882 исполняет настоящие AVM1-записи align и вызовы методов/свойств в интерпретаторе, attached native code и automatic JIT. Justify/right дают ширины 90/72 при одинаковой auto-высоте 94. Четыре OS-потока с независимыми шрифтами выполняют по 64 сборки с разными ширинами и дробным трекингом, проверяя позиции глифов и независимые контуры.

v0.883 добавляет построчно аннотированный C89-пример `--justify`; проверка примера исполняет наблюдение двух строк 90/60px. Путь остаётся отдельно принадлежащим вызывающему после освобождения шрифта и раскладки.

Проверки v0.884: прошли 96 native/tutorial, девять целевых ASan/UBSan и 12 NO_FONTS тестов. Семь переборов отказов памяти выполнены в обычной сборке. Доступность native JIT проверена отдельно, AVM1-тест проверяет счётчики входов в native-код. LeakSanitizer выключен из-за недоступности /proc; проверка утечек не заявляется. Копия отслеживаемых файлов с переименованным компонентом конфигурируется; скопированная проверка примера проходит на native-программе, отдельная detached-сборка не заявляется. Проверки исходных ссылок и аннотированных примеров проходят. Полное Unicode/bidi-выравнивание, сравнение с Flash, устройства/платформы и все девять стадий остаются открыты.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_TEXT_ALIGN_JUSTIFY` | enum constant | [include/flashvm/text_layout.h:28](../include/flashvm/text_layout.h#L28) |
| `fvm_text_layout_create` | function | [include/flashvm/text_layout.h:94](../include/flashvm/text_layout.h#L94) |
| `fvm_text_layout_create_configured` | function | [include/flashvm/text_layout.h:106](../include/flashvm/text_layout.h#L106) |
| `fvm_text_layout_create_spaced` | function | [include/flashvm/text_layout.h:122](../include/flashvm/text_layout.h#L122) |
| `fvm_text_layout_destroy` | function | [include/flashvm/text_layout.h:99](../include/flashvm/text_layout.h#L99) |
| `FVM_TextLayout` | struct | [src/capi/text_layout_c.cpp:15](../src/capi/text_layout_c.cpp#L15) |
| `FVM_TextLayoutOptions` | struct | [include/flashvm/text_layout.h:33](../include/flashvm/text_layout.h#L33) |
<!-- source-links:end -->
