# Отступ первой строки абзаца

v0.893 добавляет [fvm_text_layout_create_indented](../include/flashvm/text_layout.h#L119). Аргументы — заимствованный
шрифт, UTF-8 с явной длиной, параметры раскладки, флаги shaping и letterSpacing
как у [fvm_text_layout_create_spaced](../include/flashvm/text_layout.h#L101), плюс конечный first_line_indent в
[-4096,4096] native-пикселей. C API принимает дробные отступы. Публичные
структуры и прежние сигнатуры не меняются; ноль сохраняет прежнее поведение.

В каждом непустом абзаце смещается только первая визуальная строка. CRLF остаётся
одним разделителем; остальные существующие явные разделители и конечные пустые
строки сохраняют диапазоны. Пустые абзацы не смещаются. Перенос/выравнивание
первой строки использует max_width минус indent с минимумом 1/64 пикселя;
продолжения используют полную ширину. Слишком широкий неделимый grapheme всё
равно обеспечивает продвижение. Нулевая ширина отключает перенос. Ненулевой
отступ пока требует LTR и нулевых bidi-уровней. Поддержанные сложные слова
сохраняют shaping; bidi явно возвращает UNSUPPORTED. Это не полное соответствие
абзацев Flash/Unicode.

Line.x, позиции глифов, экспортируемые пути и подчёркивание включают отступ.
Line.advance измеряет глифы без отступа. Metrics.advance_width — максимум
max(0, advance + отступ первой строки) без выравнивающего заполнения. Отрицательный
отступ может разместить native-контуры при x<0; viewport выбирает вызывающий.
Высота, исходные кластеры и диапазоны байтов сохраняются. Границы координат и
бюджета shaping проверяются.

[FVM_TextLayout](../src/capi/text_layout_c.cpp#L13) владеет копиями глифов, метаданными и контурами; освобождение —
[fvm_text_layout_destroy](../include/flashvm/text_layout.h#L82). [fvm_text_layout_path](../include/flashvm/text_layout.h#L132)/fvm_text_layout_glyph_path
возвращают независимые [FVM_Path](../src/capi/path_c.cpp#L26), освобождаемые [fvm_path_destroy](../include/flashvm/path.h#L52) и способные
пережить шрифт/раскладку. Шрифт, текст и параметры заимствуются только на вызов.
VM/GC не участвуют. Изменяемый шрифт требует сериализации доступа; неизменяемая
раскладка допускает конкурентное чтение.

Отсутствующий output/шрифт, неверные флаги/параметры, неконечные или выходящие
за границы интервалы/отступы дают INVALID_ARGUMENT. Bidi, сложность и превышение
координат — UNSUPPORTED; отказ памяти — OUT_OF_MEMORY. Ошибки обнуляют *out.
NO_FONTS проверяет только флаги/интервалы/отступ и возвращает UNSUPPORTED без
чтения шрифта/текста/параметров. C89/C++98 и существующие ограничения байтов и
скаляров сохраняются. Платформы и сравнение с оригинальным Flash проверяются отдельно.

v0.895 применяет целый TextFormat.indent в [-4096,4096] к текущему/будущему
однородным стилям, getTextExtent и авторазмеру. Положительные значения работают
в существующем подмножестве SWF6+. Отрицательные требуют unloaded host/SWF8 и
достаточного leftMargin + blockIndent, чтобы начало первой строки оставалось
внутри левого gutter; иначе UNSUPPORTED. Дробные/неконечные/выходящие за границы
значения дают INVALID_ARGUMENT. Null/undefined сохраняют стиль. Ненулевой отступ
проверяет подготовленную раскладку до публикации ручной/автоматической правки.
Чтение кэша повторно проверяет версионный допуск отрицательных значений.
TextWidth/extent.width включают отступ первой строки, исключая поля абзаца;
autoSize включает поля и gutter как раньше.

[FVM_TEXT_STYLE_HAS_INDENT](../src/standard/text_constants.h#L23) сохраняет необязательное знаковое число после полей
абзаца. Старые записи получают ноль; публичные структуры и версия wire прежние.
Текущий/будущий стили передаются отдельно; старые readers отвергают новый флаг.
Восстановление проверяет соответствие флага, целые границы и отрицательную
политику до импорта шрифта этого стиля. Ресурс присоединяется после обоих стилей
и геометрии. Уже принятые шрифты сохраняют существующее время жизни импорта графа.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_Path` | struct | [src/capi/path_c.cpp:26](../src/capi/path_c.cpp#L26) |
| `fvm_path_destroy` | function | [include/flashvm/path.h:52](../include/flashvm/path.h#L52) |
| `fvm_text_layout_create_indented` | function | [include/flashvm/text_layout.h:119](../include/flashvm/text_layout.h#L119) |
| `fvm_text_layout_create_spaced` | function | [include/flashvm/text_layout.h:101](../include/flashvm/text_layout.h#L101) |
| `fvm_text_layout_destroy` | function | [include/flashvm/text_layout.h:82](../include/flashvm/text_layout.h#L82) |
| `fvm_text_layout_glyph_path` | function | [include/flashvm/text_layout.h:147](../include/flashvm/text_layout.h#L147) |
| `fvm_text_layout_path` | function | [include/flashvm/text_layout.h:132](../include/flashvm/text_layout.h#L132) |
| `FVM_TEXT_STYLE_HAS_INDENT` | macro / define | [src/standard/text_constants.h:23](../src/standard/text_constants.h#L23) |
| `FVM_TextLayout` | struct | [src/capi/text_layout_c.cpp:13](../src/capi/text_layout_c.cpp#L13) |
<!-- source-links:end -->
