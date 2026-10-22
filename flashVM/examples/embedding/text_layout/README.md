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
| [`fvm_font_destroy`](../../../include/flashvm/font.h#L54) | Release the owned face after layout creation; copied layout/path storage survives. No VM or device. [EN](../../../doc/TEXT_KERNING.md) / [RU](../../../doc/TEXT_KERNING.ru.md). |
| [`fvm_font_read`](../../../include/flashvm/font.h#L51) | Copy and validate embedded TTF at 40 pixels; caller destroys the returned single-owner font. Errors publish null. [EN](../../../doc/TEXT_KERNING.md) / [RU](../../../doc/TEXT_KERNING.ru.md). |
| [`fvm_path_destroy`](../../../include/flashvm/path.h#L52) | Release an owned copied path; null is accepted. All path borrows must end before destruction. [EN](../../../doc/TEXT_KERNING.md) / [RU](../../../doc/TEXT_KERNING.ru.md). |
| [`fvm_path_draw`](../../../include/flashvm/path.h#L81) | Synchronously rasterize copied contours into the caller RGBA buffer; caller owns pixels and paths. No device or VM. [EN](../../../doc/TEXT_KERNING.md) / [RU](../../../doc/TEXT_KERNING.ru.md). |
| [`fvm_path_transform`](../../../include/flashvm/path.h#L65) | Modify the owned copied contours with the explicit affine matrix; font/layout are unaffected. [EN](../../../doc/TEXT_KERNING.md) / [RU](../../../doc/TEXT_KERNING.ru.md). |
| [`fvm_text_layout_create_spaced`](../../../include/flashvm/text_layout.h#L93) | Borrow font, counted UTF-8 and options during synchronous bounded shaping with flags and signed pixel spacing; return an owned independent layout. Invalid flags/spacing clear output; ambiguous nonzero clusters are UNSUPPORTED. No VM/GC; serialize shared mutable font access. [EN](../../../doc/TEXT_SPACING.md) / [RU](../../../doc/TEXT_SPACING.ru.md). |
| [`fvm_text_layout_destroy`](../../../include/flashvm/text_layout.h#L75) | Release layout glyph/outline storage after readers finish; independent exported paths survive. [EN](../../../doc/TEXT_KERNING.md) / [RU](../../../doc/TEXT_KERNING.ru.md). |
| [`fvm_text_layout_glyph_path`](../../../include/flashvm/text_layout.h#L118) | Copy one positioned glyph outline into a new owned path; caller destroys it. No font or VM is needed. [EN](../../../doc/TEXT_KERNING.md) / [RU](../../../doc/TEXT_KERNING.ru.md). |
| [`fvm_text_layout_line`](../../../include/flashvm/text_layout.h#L99) | Copy line bounds, baseline and direction by ordinal without allocation; layout remains borrowed. [EN](../../../doc/TEXT_KERNING.md) / [RU](../../../doc/TEXT_KERNING.ru.md). |
| [`fvm_text_layout_metrics`](../../../include/flashvm/text_layout.h#L97) | Copy whole-layout advances, ink bounds and counts without allocation; no VM or device. [EN](../../../doc/TEXT_KERNING.md) / [RU](../../../doc/TEXT_KERNING.ru.md). |
| [`fvm_text_layout_path`](../../../include/flashvm/text_layout.h#L105) | Copy complete placed contours into a new owned path; caller releases it independently of layout. [EN](../../../doc/TEXT_KERNING.md) / [RU](../../../doc/TEXT_KERNING.ru.md). |

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
ownership and concurrency: [EN](../../../doc/TEXT_SPACING.md) / [RU](../../../doc/TEXT_SPACING.ru.md).

Эти режимы используют AV без кернинга: +2.5 пикселя после каждого символа дают
одну строку, два глифа и advance 53.00; -2 дают 44.00. Оба выполняют экспорт
независимых контуров и отрисовку RGBA, как основной режим. Сложные кластеры
не входят в поддержку ненулевого интервала; окна и устройства не используются.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_font_destroy` | function | [include/flashvm/font.h:54](../../../include/flashvm/font.h#L54) |
| `fvm_font_read` | function | [include/flashvm/font.h:51](../../../include/flashvm/font.h#L51) |
| `fvm_path_destroy` | function | [include/flashvm/path.h:52](../../../include/flashvm/path.h#L52) |
| `fvm_path_draw` | function | [include/flashvm/path.h:81](../../../include/flashvm/path.h#L81) |
| `fvm_path_transform` | function | [include/flashvm/path.h:65](../../../include/flashvm/path.h#L65) |
| `fvm_text_layout_create_spaced` | function | [include/flashvm/text_layout.h:93](../../../include/flashvm/text_layout.h#L93) |
| `fvm_text_layout_destroy` | function | [include/flashvm/text_layout.h:75](../../../include/flashvm/text_layout.h#L75) |
| `fvm_text_layout_glyph_path` | function | [include/flashvm/text_layout.h:118](../../../include/flashvm/text_layout.h#L118) |
| `fvm_text_layout_line` | function | [include/flashvm/text_layout.h:99](../../../include/flashvm/text_layout.h#L99) |
| `fvm_text_layout_metrics` | function | [include/flashvm/text_layout.h:97](../../../include/flashvm/text_layout.h#L97) |
| `fvm_text_layout_path` | function | [include/flashvm/text_layout.h:105](../../../include/flashvm/text_layout.h#L105) |
<!-- source-links:end -->
