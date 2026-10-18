# Документация и примеры / Documentation and examples

Постоянное требование пользователя закреплено в [AGENTS.md](../AGENTS.md).
Оно применяется к дальнейшей разработке, а не только к текущим документам.

## Ссылки на исходники

Для описываемой функции, callback-типа, структуры, enum, макроса, define и
именованной константы указывайте реальное объявление/определение в исходниках.
Для C API предпочтительно публичное объявление; для структуры — полное
определение, а не только opaque forward declaration. Если пониманию помогает
реализация, добавьте отдельную ссылку на неё. Переходы к руководству и к
исходникам дополняют друг друга: ссылка на header не заменяет описание API.

Примеры символов: [`fvm_crash_configure`](../include/flashvm/crash.h#L55), [`FVM_CrashFrame`](../include/flashvm/crash.h#L32), [`FVM_CRASH_DEBUGGER`](../include/flashvm/crash.h#L16),
[`FVM_API`](../include/flashvm/flashvm.h#L16), [`FVM_ABI_VERSION`](../include/flashvm/flashvm.h#L25). Номера строк относятся к текущему checkout;
в исторических заметках ссылка также ведёт к текущему определению.

```sh
python3 scripts/link_documentation_sources.py
python3 scripts/link_documentation_sources.py --check
python3 scripts/check_debugger_examples.py
```

Команды выполняются из корня проекта. Первая обновляет inline source links и
таблицы «Исходники / Source declarations» в README и doc/examples Markdown;
вторая ничего не пишет и завершает работу с ошибкой, если обновление необходимо.
Третья проверяет существующий контракт debugger tutorials. Ссылки на разделы
руководств и заголовки сохраняются, содержимое fenced code не превращается в
Markdown-ссылки. После сдвига строк исходника обновляются и прежние source links.

Инструмент использует только разрешённые по объявлениям project-символы и
именованные DEFS switches из CMake. Generic локальные имена, удалённые API,
неизвестные символы и исторические псевдонимы не сопоставляются наугад:
укажите корректный файл вручную либо явно объясните отсутствие определения.
Автоматическое сопоставление не заменяет проверку смысла ссылки редактором.

## Описание API

У каждой публичной функции должны быть назначение, параметры, результаты и
ошибки, владение и время жизни возвращаемых данных, ограничения потока/GC/
повторного входа и особенности реализации, важные вызывающему коду. Для типов
объясняйте поля; для макросов и констант — значение и область применения.
Примеры в руководстве должны соответствовать поддерживаемому контракту.
Русская и английская версии не должны противоречить друг другу.

## Каталог и README примеров

Поддерживайте [examples/README.md](../examples/README.md): основные группы по
папкам, логические подгруппы по одной идее, алфавитную сортировку на каждом
уровне, оглавление с переходами к описаниям и финальный плоский алфавитный
глоссарий со всеми примерами. Обновляйте его вместе с добавлением/изменением
примера. Существующие main-программы, AS-исходники и концептуальные заметки
должны различаться по типу и требованиям запуска.

В таблице API описывайте конкретную функцию и её роль в примере: общая аннотация
раздела руководства не заменяет назначение функции. Отдельно указывайте ссылку
на контракт функции, владение и существенные блокировки.

README каждого примера описывает:

- зачем он нужен, что позволяет увидеть и какой результат ожидается;
- исходники, зависимости, профиль DEFS и точные команды сборки/запуска;
- устройство, callback-соглашения, владение, очистку и побочные эффекты;
- демонстрируемые C/script API со ссылками на исходники и конкретные разделы руководств;
- ограничения и отличие реального запуска от эскиза или концептуальной заметки.

Код примера форматируется для чтения, каждая непустая строка комментируется,
ошибки обрабатываются обычным control flow без assert. Companion headers и
AS-файлы описываются в составе своего примера. Для одиночного .as допустим
соседний .md с описанием и ссылка из README папки.

В оглавлении каталога после имени каждого примера добавляйте краткую аннотацию
в `<small><em>…</em></small>`: меньший шрифт и курсив отделяют её от ссылки.
Формулировка должна быть короткой, обычно на одну, максимум на две строки.

## Проверка перед коммитом

Проверяйте полноту каталога, порядок подгрупп/пунктов/глоссария, ссылки на файлы,
Markdown anchors и реальные строки исходников. При изменениях кода запускайте
соответствующие runtime-проверки; для чистой документации достаточно проверки
документов и затронутых метаданных версии. Не выдавайте старые runtime-прогоны
за повторённые. Правила версии, master и зеркала остаются в AGENTS.md.

## English

This is a standing maintenance contract. Link documented functions, callback
and aggregate types, macros/defines and constants to actual source declarations
or definitions, with current line anchors where supported. Refresh generated
links after source changes and run the read-only check. Keep manual/TOC links,
heading anchors and fenced example code intact. Never invent a source location
for an unresolved/historical symbol; review automatic matches for relevance.

API tables must describe each function, not repeat the guide section summary.
Link the function contract and explain ownership and relevant blocking behavior.

Every public API needs purpose, parameters, results/errors, ownership/lifetime,
thread/GC/reentry restrictions and material limits. Every example needs a README
with purpose, requirements, implementation, cleanup, exact build/run commands,
expected output, limits and guide/source links for demonstrated APIs. Explain
every nonblank tutorial code line and use ordinary error handling, without asserts.
Maintain the folder-based catalog, alphabetic thematic subgroups and entries,
linked structured contents and final flat alphabetical glossary. Validate links,
coverage, ordering and target requirements; scale runtime checks to actual code
changes and report evidence accurately.

## HTML anchors and attributes / HTML-якоря и атрибуты

The source linker preserves raw HTML tags and comments byte-for-byte, including
multiline tags and quoted `>` characters. HTML attribute values are not prose:
never insert Markdown links into an `id`, `href`, `title` or another attribute.
Text between HTML tags still receives normal source links. Fenced samples remain
unchanged. The six-seed regression covers these cases and repeated refresh.

Генератор сохраняет HTML-теги и комментарии без изменений, включая многострочные
теги и `>` внутри кавычек. Значения атрибутов не являются обычным текстом:
Markdown-ссылки внутри `id`, `href` и `title` ломают якоря и навигацию. Текст
между тегами по-прежнему получает ссылки; fenced-примеры не меняются. Проверка
на шести hash seeds контролирует эти случаи и повторное обновление.

## Deterministic source table order / стабильный порядок

Generated declaration rows sort by casefolded name and then original spelling.
The second key distinguishes a macro such as [FVM_SWF_SOUND_EVENT_ENVELOPE](../include/flashvm/swf_sound.h#L32) from
[fvm_swf_sound_event_envelope](../include/flashvm/swf_sound.h#L60) without depending on Python hash iteration order.
`scripts/test_documentation_source_links.py` checks six hash seeds and repeated
refresh; it is registered in native CTest. The fixture stays in ignored build
output on the source drive so Windows temporary-drive differences do not alter
relative-source-link behavior. Continue running the normal linker's --check.

Таблицы исходников сортируются по casefold и затем исходному написанию: одноимённые
макрос/функция с разным регистром получают стабильный порядок. Регрессия проверяет
шесть hash seeds и idempotence; проверка включена в CTest. Генерируемые fixtures
живут в ignored build output, не в исходниках/истории. Обычный --check обязателен.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_ABI_VERSION` | macro / define | [include/flashvm/flashvm.h:25](../include/flashvm/flashvm.h#L25) |
| `FVM_API` | macro / define | [include/flashvm/flashvm.h:16](../include/flashvm/flashvm.h#L16) |
| `fvm_crash_configure` | function | [include/flashvm/crash.h:55](../include/flashvm/crash.h#L55) |
| `FVM_CRASH_DEBUGGER` | enum constant | [include/flashvm/crash.h:16](../include/flashvm/crash.h#L16) |
| `FVM_CrashFrame` | struct | [include/flashvm/crash.h:32](../include/flashvm/crash.h#L32) |
| `FVM_SWF_SOUND_EVENT_ENVELOPE` | macro / define | [include/flashvm/swf_sound.h:32](../include/flashvm/swf_sound.h#L32) |
| `fvm_swf_sound_event_envelope` | function | [include/flashvm/swf_sound.h:60](../include/flashvm/swf_sound.h#L60) |
<!-- source-links:end -->
