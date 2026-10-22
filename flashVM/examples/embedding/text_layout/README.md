# embedding/text_layout

Создаёт горизонтальную раскладку текста, запрашивает метрики и строки и извлекает контуры отдельных glyph и всего layout. Показывает преобразование и отрисовку owned-путей и порядок освобождения font/layout/path.

## Реализация и запуск

Пример C/C++ собирается с `FVM_BUILD_EXAMPLES=ON`. Из корня репозитория:

```sh
cmake -S . -B build-examples -DFVM_DEFS_FILE=cmake/profiles/agg.defs -DFVM_BUILD_EXAMPLES=ON
cmake --build build-examples --target flashvm_examples_embedding_text_layout_main --parallel
./build-examples/flashvm_examples_embedding_text_layout_main
```

Операции зависят от выбранного DEFS-профиля; OS_PORTS, JIT, графика и codecs
не включаются наличием этого README. Исходники ниже показывают порядок API-вызовов
и освобождения ресурсов.

<!-- example-catalog-api:start -->
**Текущая CMake-цель:** `flashvm_examples_embedding_text_layout_main`.


## Демонстрируемые функции C API

Имена ведут к публичным декларациям. Во второй колонке указан файл руководства
и раздел с контрактом; общие функции setup/cleanup включены для воспроизводимости.

| Функция | Где читать подробнее |
| --- | --- |
| [`fvm_font_destroy`](../../../include/flashvm/font.h#L56) | Release the owned face after layout creation; copied layout/path storage survives. No VM or device. [EN](../../../doc/TEXT_KERNING.md) / [RU](../../../doc/TEXT_KERNING.ru.md). |
| [`fvm_font_underline_metrics`](../../../include/flashvm/font.h#L65) | Borrow the font and copy scaled center/thickness before face destruction; missing metrics fail explicitly with cleared output. No allocation/VM/GC, serialized font access. [EN](../../../doc/TEXT_UNDERLINE.md) / [RU](../../../doc/TEXT_UNDERLINE.ru.md). |
| [`fvm_font_read`](../../../include/flashvm/font.h#L53) | Copy and validate embedded TTF at 40 pixels; caller destroys the returned single-owner font. Errors publish null. [EN](../../../doc/TEXT_KERNING.md) / [RU](../../../doc/TEXT_KERNING.ru.md). |
| [`fvm_path_append`](../../../include/flashvm/path.h#L67) | Borrow destination/source paths and identity transform; synchronously copy line bars into caller-owned geometry, then release the temporary source. Status is checked before continuation. [EN](../../../doc/TEXT_UNDERLINE.md) / [RU](../../../doc/TEXT_UNDERLINE.ru.md). |
| [`fvm_path_count`](../../../include/flashvm/path.h#L56) | Read an owned path command count without allocation or mutation, to measure the added rectangle commands. No VM/GC or retained borrow. [EN](../../../doc/TEXT_UNDERLINE.md) / [RU](../../../doc/TEXT_UNDERLINE.ru.md). |
| [`fvm_path_destroy`](../../../include/flashvm/path.h#L52) | Release an owned copied path; null is accepted. All path borrows must end before destruction. [EN](../../../doc/TEXT_KERNING.md) / [RU](../../../doc/TEXT_KERNING.ru.md). |
| [`fvm_path_draw`](../../../include/flashvm/path.h#L81) | Synchronously rasterize copied contours into the caller RGBA buffer; caller owns pixels and paths. No device or VM. [EN](../../../doc/TEXT_KERNING.md) / [RU](../../../doc/TEXT_KERNING.ru.md). |
| [`fvm_path_transform`](../../../include/flashvm/path.h#L65) | Modify the owned copied contours with the explicit affine matrix; font/layout are unaffected. [EN](../../../doc/TEXT_KERNING.md) / [RU](../../../doc/TEXT_KERNING.ru.md). |
| [`fvm_text_layout_create_spaced`](../../../include/flashvm/text_layout.h#L101) | Borrow font, counted UTF-8 and options during synchronous bounded shaping with flags and signed pixel spacing; return an owned independent layout. Nonzero spacing disables optional liga/clig; required/ambiguous clusters are UNSUPPORTED. Invalid flags/spacing clear output. No VM/GC; serialize shared mutable font access. [EN](../../../doc/TEXT_LIGATURE_TRACKING.md) / [RU](../../../doc/TEXT_LIGATURE_TRACKING.ru.md). |
| [`fvm_text_layout_destroy`](../../../include/flashvm/text_layout.h#L82) | Release layout glyph/outline storage after readers finish; independent exported paths survive. [EN](../../../doc/TEXT_KERNING.md) / [RU](../../../doc/TEXT_KERNING.ru.md). |
| [`fvm_text_layout_glyph_path`](../../../include/flashvm/text_layout.h#L128) | Copy one positioned glyph outline into a new owned path; caller destroys it. No font or VM is needed. [EN](../../../doc/TEXT_KERNING.md) / [RU](../../../doc/TEXT_KERNING.ru.md). |
| [`fvm_text_layout_line`](../../../include/flashvm/text_layout.h#L107) | Copy line bounds, baseline and direction by ordinal without allocation; layout remains borrowed. [EN](../../../doc/TEXT_KERNING.md) / [RU](../../../doc/TEXT_KERNING.ru.md). |
| [`fvm_text_layout_line_underline_path`](../../../include/flashvm/text_layout.h#L125) | Borrow a placed layout, line index and copied center/thickness; return an independent bar path or an owned empty path for zero advance. Errors clear output; caller destroys it. Independent immutable readers are allowed. [EN](../../../doc/TEXT_UNDERLINE.md) / [RU](../../../doc/TEXT_UNDERLINE.ru.md). |
| [`fvm_text_layout_metrics`](../../../include/flashvm/text_layout.h#L105) | Copy whole-layout advances, ink bounds and counts without allocation; no VM or device. [EN](../../../doc/TEXT_KERNING.md) / [RU](../../../doc/TEXT_KERNING.ru.md). |
| [`fvm_text_layout_path`](../../../include/flashvm/text_layout.h#L113) | Copy complete placed contours into a new owned path; caller releases it independently of layout. [EN](../../../doc/TEXT_KERNING.md) / [RU](../../../doc/TEXT_KERNING.ru.md). |

[Описание в общем каталоге](../../README.md#example-embedding-text_layout).
<!-- example-catalog-api:end -->


## Explicit kerning / Явный кернинг (v0.824)

```sh
cmake -S . -B build-text -DFVM_DEFS_FILE=cmake/profiles/text-jit.defs -DCMAKE_BUILD_TYPE=Release
cmake --build build-text --target flashvm_examples_embedding_text_layout_main
./build-text/flashvm_examples_embedding_text_layout_main
./build-text/flashvm_examples_embedding_text_layout_main --no-kerning
```

Default invocation preserves legacy native kerning. `--no-kerning` disables pair
kerning consistently during wrapping, measurements, placed paths and software
rasterization. Ligatures and bidi shaping remain enabled. The original CC0 geometric
font has an AV adjustment of -80/1000 em; at 40 pixels the first-line advance changes
by about 3.2 pixels. The example prints line/glyph metadata and draws into an owned
RGBA memory buffer, without opening a window or writing files. Unknown options fail
before native allocation. NO_FONTS prints its existing prerequisite message.

По умолчанию сохраняется прежний нативный кернинг. `--no-kerning` отключает его
одинаково для переносов, метрик, путей и программного рендера. Лигатуры и bidi
сохраняются. Оригинальный CC0-шрифт задаёт AV -80/1000 em: при 40 px разница
составляет около 3.2 px. Пример печатает метаданные и рисует в собственный RGBA-буфер
без окон и файлов. Неизвестные параметры отвергаются до выделения; NO_FONTS печатает
прежнее требование FONTS. Это не приёмка устройств и не полный SWF TextField.

## Signed tracking / Знаковый интервал (v0.835)

```sh
./build-text/flashvm_examples_embedding_text_layout_main --tracking
./build-text/flashvm_examples_embedding_text_layout_main --condensed
```

These modes use simple AV without kerning: +2.5 pixels after each character gives
one line, two glyphs and advance 53.00; -2 gives 44.00. Both execute the same owned
path export and RGBA rasterization as the default mode. Complex clusters are
explicitly outside nonzero tracking support. Full API arguments, statuses,
ownership and concurrency: [EN](../../../doc/TEXT_LIGATURE_TRACKING.md) / [RU](../../../doc/TEXT_LIGATURE_TRACKING.ru.md).

Эти режимы используют AV без кернинга: +2.5 пикселя после каждого символа дают
одну строку, два глифа и advance 53.00; -2 дают 44.00. Оба выполняют экспорт
независимых контуров и отрисовку RGBA, как основной режим. Сложные кластеры
не входят в поддержку ненулевого интервала; окна и устройства не используются.

## Underline / Подчёркивание (v0.843)

```sh
./build-text/flashvm_examples_embedding_text_layout_main --underline
```

The original three-line sample keeps identical glyph/byte/bidi metadata and
metrics. Copied font metrics are center=4.00/thickness=2.00; three closed bars
add 15 path commands. The font and layout are destroyed before RGBA rendering.
An interior pixel (1,40) independently confirms alpha 255 below glyph outlines.
All storage is caller-owned; no window, file or physical device is used.
[Contracts](../../../doc/TEXT_UNDERLINE.md).

Исходный текст из трёх строк сохраняет глифы, метаданные байтов/bidi и метрики.
Скопированные метрики: центр 4.00, толщина 2.00; три полосы добавляют 15 команд.
Шрифт и раскладка уничтожены до RGBA-отрисовки. Пиксель (1,40) подтверждает
alpha 255 ниже контуров глифов. Память принадлежит примеру, окон и устройств нет.
[Контракты](../../../doc/TEXT_UNDERLINE.ru.md).

## Optional ligatures / Необязательные лигатуры (v0.867)

Run the same binary with `--no-ligatures` or `--ligature-tracking`. The first
passes [FVM_FONT_SHAPE_NO_OPTIONAL_LIGATURES](../../../include/flashvm/font.h#L10) at zero spacing; the second requests
+1 tracking with flags zero. Both output one line and two glyphs. With the 40px
CC0 font, advance is respectively 24.00 or 26.00. The owned path is exported after
font destruction and rasterized by the existing native example. No device/window
is involved. Required/ambiguous clusters remain unsupported with nonzero spacing.
The fixture, build target, cleanup and threading requirements above are unchanged.

Режим `--no-ligatures` передаёт [FVM_FONT_SHAPE_NO_OPTIONAL_LIGATURES](../../../include/flashvm/font.h#L10) с нулевым
интервалом; `--ligature-tracking` задаёт +1 с нулевыми флагами. Оба дают одну
строку и два глифа; при 40px advance равен 24.00 и 26.00 соответственно.
Независимый контур экспортируется после уничтожения шрифта и растеризуется без
окна/устройства. Обязательные/неоднозначные кластеры при ненулевом трекинге ещё
не поддержаны. Владение, сборка и потоки описаны выше.

Contracts/API ownership and statuses: [EN](../../../doc/TEXT_LIGATURE_TRACKING.md)
/ [RU](../../../doc/TEXT_LIGATURE_TRACKING.ru.md).

## Justified paragraph / Абзац по ширине (v0.883)

Run `--justify` with the same text-jit build. The C89 tutorial selects
[FVM_TEXT_ALIGN_JUSTIFY](../../../include/flashvm/text_layout.h#L24), LTR, inner width 90 and the original CC0 40px font. It
prints `lines=2 glyphs=7 advance=90.00 height=92.00` and
`justify first=90.00 last=60.00`. The second line remains natural. The existing
font destruction, independent layout/path export and native rasterization still
run; no window/device is required. Only unambiguous U+0020 gaps in LTR paragraphs
are justified; exact Flash/CJK/bidi conformance remains open.

`--justify` в той же text-jit сборке выбирает [FVM_TEXT_ALIGN_JUSTIFY](../../../include/flashvm/text_layout.h#L24), LTR и
внутреннюю ширину 90 при исходном CC0-шрифте 40px. Вывод: две строки, семь
глифов, advance 90.00 и высота 92.00; первая строка 90.00, последняя 60.00.
Прежний порядок font/layout/path и растеризация выполняются без окна/устройства.
Поддержаны однозначные U+0020 в LTR; точная совместимость Flash/CJK/bidi открыта.

[fvm_text_layout_line](../../../include/flashvm/text_layout.h#L107) borrows an immutable layout and copies scalar/byte/glyph
ranges into caller storage; no allocation or ownership transfer occurs. Invalid
layout/output/index returns INVALID_ARGUMENT. No VM/GC is involved; independent
readers may query immutable layout data. The three layout constructors borrow
font/text/options only during construction, return owned layouts and clear the
output on failure; flags/spacing, statuses, threading and scope are detailed in
[EN](../../../doc/TEXT_JUSTIFY.md) / [RU](../../../doc/TEXT_JUSTIFY.ru.md).

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_font_destroy` | function | [include/flashvm/font.h:56](../../../include/flashvm/font.h#L56) |
| `fvm_font_read` | function | [include/flashvm/font.h:53](../../../include/flashvm/font.h#L53) |
| `FVM_FONT_SHAPE_NO_OPTIONAL_LIGATURES` | macro / define | [include/flashvm/font.h:10](../../../include/flashvm/font.h#L10) |
| `fvm_font_underline_metrics` | function | [include/flashvm/font.h:65](../../../include/flashvm/font.h#L65) |
| `fvm_path_append` | function | [include/flashvm/path.h:67](../../../include/flashvm/path.h#L67) |
| `fvm_path_count` | function | [include/flashvm/path.h:56](../../../include/flashvm/path.h#L56) |
| `fvm_path_destroy` | function | [include/flashvm/path.h:52](../../../include/flashvm/path.h#L52) |
| `fvm_path_draw` | function | [include/flashvm/path.h:81](../../../include/flashvm/path.h#L81) |
| `fvm_path_transform` | function | [include/flashvm/path.h:65](../../../include/flashvm/path.h#L65) |
| `FVM_TEXT_ALIGN_JUSTIFY` | enum constant | [include/flashvm/text_layout.h:24](../../../include/flashvm/text_layout.h#L24) |
| `fvm_text_layout_create_spaced` | function | [include/flashvm/text_layout.h:101](../../../include/flashvm/text_layout.h#L101) |
| `fvm_text_layout_destroy` | function | [include/flashvm/text_layout.h:82](../../../include/flashvm/text_layout.h#L82) |
| `fvm_text_layout_glyph_path` | function | [include/flashvm/text_layout.h:128](../../../include/flashvm/text_layout.h#L128) |
| `fvm_text_layout_line` | function | [include/flashvm/text_layout.h:107](../../../include/flashvm/text_layout.h#L107) |
| `fvm_text_layout_line_underline_path` | function | [include/flashvm/text_layout.h:125](../../../include/flashvm/text_layout.h#L125) |
| `fvm_text_layout_metrics` | function | [include/flashvm/text_layout.h:105](../../../include/flashvm/text_layout.h#L105) |
| `fvm_text_layout_path` | function | [include/flashvm/text_layout.h:113](../../../include/flashvm/text_layout.h#L113) |
<!-- source-links:end -->
