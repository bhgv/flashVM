# Явное управление кернингом

[English](TEXT_KERNING.md).

## Нативное формирование (v0.820)

[fvm_font_shape_configured](../include/flashvm/font.h#L90) добавляет flags к прежнему контракту владения native
run. Ноль сохраняет HarfBuzz defaults; [FVM_FONT_SHAPE_NO_KERNING](../include/flashvm/font.h#L9) отключает kern,
сохраняя лигатуры и размещение marks. Старый [fvm_font_shape](../include/flashvm/font.h#L76) не меняется.
Неизвестные flags дают INVALID_ARGUMENT и очищают выходной owner. В NO_FONTS
корректные flags дают UNSUPPORTED. UTF-8, direction и владение font соответствуют
прежнему API; font handle нельзя использовать конкурентно. Возвращённый run
независим. Нет VM, GC, callbacks и устройств.


## Размещение и переносы (v0.821)

[fvm_text_layout_create_configured](../include/flashvm/text_layout.h#L89) принимает те же shaping flags без изменения
[FVM_TextLayoutOptions](../include/flashvm/text_layout.h#L29) и старого create. Все пробные строки и конечные script/bidi
сегменты используют одинаковые features: ширина, выравнивание, позиции и пути
согласованы. Результат владеет скопированными путями и переживает font; сам font
остаётся однопоточным. Ошибки очищают результат, неизвестные flags дают
INVALID_ARGUMENT, корректные NO_FONTS-запросы — UNSUPPORTED. Табуляция, styled spans
и остальные прежние ограничения не снимаются.


## AS2 TextFormat/TextField (v0.822)

Новые TextFormat и динамические TextField по контракту AS2 Flash 8 имеют
kerning=false. Явные non-null значения проходят VM Boolean conversion;
null/undefined сохраняют прежний стиль при merge. Текущий и будущий форматы
независимы, getters возвращают Boolean; редактирование, кеш метрик и экспорт
путей используют одну настройку. Styled spans, полные device fonts и весь
TextField остаются вне этого subset. Старые геометрические fixtures теперь
явно включают ранее неявный кернинг.


## Совместимость переноса (v0.823)

Текущий/будущий стили поля сохраняют кернинг через binary serialization и channels.
Прежнее слово alignment несёт явный NO_KERNING-флаг без изменения длины записи,
memory charge и публичных ABI-структур. Старые записи без флага сохраняют прежний
включённый кернинг; новые поля по умолчанию выключают его. Неизвестные биты и
неверные alignment отвергаются с прежним rollback. Старые readers отвергают новый
флаг вместо молчаливого изменения ширины. Эта совместимость field records не
сохраняет неявное поведение старых общих TextFormat-объектов с null-флагами.


## Растровый пример C89 (v0.824)

[Пример text layout](../examples/embedding/text_layout/README.md) принимает
`--no-kerning` и рисует независимые owned-пути в RGBA-буфер хоста. Без флага
сохраняется прежний нативный shaping. Профиль text-jit включает закреплённые
статические шрифтовые библиотеки, rasterization и доступный native JIT без
SDL/system fonts. Окно и выходной файл не создаются.


## Выполнение VM и rollback (v0.825)

Настоящие AVM1-записи свойств и вызовы методов дают одинаковые ширины в interpreter,
attached native bodies и automatic JIT; счётчики native entries подтверждают путь.
GC между вызовами сохраняет поле и формат. Перебор отказов выделения в обычной
сборке покрывает setters текущего/будущего форматов: ошибка сохраняет кернинг и
кеш ширины, roots снимаются, исправный повтор проходит. Editing-fixture теперь
ожидает равные AV/BV advances при правильном выключенном default; явный кернинг
покрыт отдельными проверками.


## C ABI и изоляция потоков (v0.826)

Оба native font shaping API проверяют байты direction enum до чтения значения
C++. Неверные C-представления дают INVALID_ARGUMENT и очищают выход также
в NO_FONTS. Четыре OS-потока независимо формируют текст с разным кернингом
в собственных font/run/layout, разделяя лишь неизменяемые шрифтовые байты;
лигатуры fi и аналитические AV-ширины сохраняются. Общий изменяемый font handle
не становится безопасным; это не TSan и не приёмка нативных платформ.

Пример использует оригинальную CC0 GPOS-пару (-80/1000 em при 40 px), проверяет
разницу 3.20 px, одинаковые bidi-строки/байтовые границы и настоящий software
rasterization. Отдельно проверяются native JIT, graph/channel transfer, GC,
rollback отказов выделения и NO_FONTS. Styled spans, letterSpacing, поля, tabs,
system fonts и весь SWF TextField остаются открытыми. [Adobe AS2 (зеркало)](https://open-flash.github.io/mirrors/as2-language-reference/TextFormat.html).


Приёмка: прошли 25 font/text/tutorial-проверок с профилем FONTS/JIT и пять
выбранных NO_FONTS-проверок ABI/ошибок. Шесть full-core ASan/UBSan включают
настоящий JIT, перенос и независимые потоки. Инъекции отказов выделения проверены
обычной сборкой и не выдаются за чистый sanitizer exception-unwind результат.
Физические устройства/платформы и полный SWF-текст этим не подтверждены.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_font_shape` | function | [include/flashvm/font.h:76](../include/flashvm/font.h#L76) |
| `fvm_font_shape_configured` | function | [include/flashvm/font.h:90](../include/flashvm/font.h#L90) |
| `FVM_FONT_SHAPE_NO_KERNING` | macro / define | [include/flashvm/font.h:9](../include/flashvm/font.h#L9) |
| `fvm_text_layout_create_configured` | function | [include/flashvm/text_layout.h:89](../include/flashvm/text_layout.h#L89) |
| `FVM_TextLayoutOptions` | struct | [include/flashvm/text_layout.h:29](../include/flashvm/text_layout.h#L29) |
<!-- source-links:end -->
