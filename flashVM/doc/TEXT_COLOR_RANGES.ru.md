# Цветовые диапазоны общей раскладки TextField

[English](TEXT_COLOR_RANGES.md) · [Асинхронная отрисовка](RENDER_ASYNC.ru.md) ·
[Публичный C API](../include/flashvm/standard_text.h)

[`fvm_textfield_path_ranges`](../include/flashvm/standard_text.h#L119) экспортирует контуры выбранных глифов из той же
полной кэшированной раскладки, что и [`fvm_textfield_path`](../include/flashvm/standard_text.h#L91). Host рисует диапазоны
разными цветами без повторного shaping каждого токена, изменения ширин и
разрезания лигатур. Это нативное расширение представления. Полная поддержка
AS TextFormat по диапазонам, смешанных шрифтов и HTML этим API не добавлена.

| API/тип | Контракт |
| --- | --- |
| [`FVM_TextPathRange`](../include/flashvm/standard_text.h#L99) | Полуоткрытый диапазон байтов UTF-8 исходного counted-текста; не UTF-16 AS и не скаляры Dis |
| [`FVM_TEXT_PATH_RANGE_LIMIT`](../include/flashvm/standard_text.h#L95) | Максимум 1024 заимствованных диапазона за вызов |
| [`fvm_textfield_path_ranges`](../include/flashvm/standard_text.h#L119) | Отдельно владеемые контуры выбранных глифов; успешный результат принадлежит вызывающему |
| [`fvm_textfield_path`](../include/flashvm/standard_text.h#L91) | Прежние полные контуры с единым подчёркиванием |

Совпадающие context/field должны быть живыми и достижимыми в потоке владельца VM.
Массив заимствован только на время вызова. Диапазоны непустые, отсортированные,
непересекающиеся, в пределах counted-текста и на границах UTF-8-скаляров.
Смежные диапазоны допустимы. Нулевой count допускает NULL-массив и возвращает
пустой отдельно владеемый путь после обычной проверки раскладки. Встроенный
NUL не завершает counted-текст.

Принадлежность определяется **началом shaping-кластера** глифа. Граница внутри
кластера не разрезает его: глиф принадлежит диапазону с началом кластера.
Визуальный bidi-порядок не считается порядком исходника; для каждого глифа
независимо ищется подходящий диапазон. Для единого цвета отображаемой графемы
приложение должно выбирать соответствующие границы графем/кластеров.

Кернинг, переносы, выравнивание и прокрутка остаются общими. В контурах уже
учтены локальный отступ поля и scroll; viewport clipping не выполняется,
host применяет свой world transform и clip ровно один раз. Единое подчёркивание
исключено из выбранных контуров, иначе оно повторялось бы каждым цветом.
Декорации рисуются отдельно или через прежний полный экспорт. Исходник, стиль,
каретка, выделение, Undo и сохранённость не изменяются. Успешное наблюдение
может ограничить stored scroll текущей геометрией, как прежний полный экспорт.
При ошибке scroll сохраняется, выходной указатель очищается.

Ошибки context/object/output/диапазонов/count/границ дают [`FVM_INVALID_ARGUMENT`](../include/flashvm/flashvm.h#L47).
Ошибки шрифта, раскладки и выделения памяти передаются вызывающему; профиль
без шрифтов остаётся unsupported. Скрипты, каналы и устройства не вызываются.
Изменения field/layout и GC сериализуются владельцем. Успешный путь освобождается
через [`fvm_path_destroy`](../include/flashvm/path.h#L64); worker не получает VM/font handles. Отделённые контуры
копируются в неизменяемое задание растеризации и переживают замену исходника.

IDE преобразует проверенные по revision скалярные диапазоны Dis в байты UTF-8
на владельце и группирует контуры по палитре. Сначала рисуется выделение,
потом цветные глифы, последней — каретка. Обновление только цветов сохраняет
номер представления/ввода и использует прежние dirty tiles и нативные workers.
Оно не подтверждает ожидающее редактирование и не ждёт кадра старого Flash.
[Ленивый сервис IDE](../../ide/SYNTAX-LAZY.ru.md) описан отдельно.

[Проверка диапазонов](../tests/textfield_path_ranges.cpp) исполняет настоящую
раскладку, проверяет разбиение контуров, UTF-8/границы/квоты/пустой результат и
независимое владение. Прежние underline/scroll/counting и JIT-проверки сохраняют
свою область приёмки. Стадии совместимости Flash и физических платформ не закрыты.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_INVALID_ARGUMENT` | enum constant | [include/flashvm/flashvm.h:47](../include/flashvm/flashvm.h#L47) |
| `fvm_path_destroy` | function | [include/flashvm/path.h:64](../include/flashvm/path.h#L64) |
| `FVM_TEXT_PATH_RANGE_LIMIT` | macro / define | [include/flashvm/standard_text.h:95](../include/flashvm/standard_text.h#L95) |
| `fvm_textfield_path` | function | [include/flashvm/standard_text.h:91](../include/flashvm/standard_text.h#L91) |
| `fvm_textfield_path_ranges` | function | [include/flashvm/standard_text.h:119](../include/flashvm/standard_text.h#L119) |
| `FVM_TextPathRange` | struct | [include/flashvm/standard_text.h:99](../include/flashvm/standard_text.h#L99) |
<!-- source-links:end -->
