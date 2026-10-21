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
| [`fvm_font_destroy`](../../../include/flashvm/font.h#L48) | [FONT_LIBRARIES.md#flashvm-c89-embedding-api](../../../doc/FONT_LIBRARIES.md#flashvm-c89-embedding-api) — Шрифты, shaping и редактируемые контуры. |
| [`fvm_font_read`](../../../include/flashvm/font.h#L45) | [FONT_LIBRARIES.md#flashvm-c89-embedding-api](../../../doc/FONT_LIBRARIES.md#flashvm-c89-embedding-api) — Шрифты, shaping и редактируемые контуры. |
| [`fvm_path_destroy`](../../../include/flashvm/path.h#L52) | [FONT_LIBRARIES.md#owned-vector-model](../../../doc/FONT_LIBRARIES.md#owned-vector-model) — Владеющие векторные пути и преобразования. |
| [`fvm_path_draw`](../../../include/flashvm/path.h#L81) | [FONT_LIBRARIES.md#owned-vector-model](../../../doc/FONT_LIBRARIES.md#owned-vector-model) — Владеющие векторные пути и преобразования. |
| [`fvm_path_transform`](../../../include/flashvm/path.h#L65) | [FONT_LIBRARIES.md#owned-vector-model](../../../doc/FONT_LIBRARIES.md#owned-vector-model) — Владеющие векторные пути и преобразования. |
| [`fvm_text_layout_create`](../../../include/flashvm/text_layout.h#L72) | [TEXT_LAYOUT.md#layout-and-measurement](../../../doc/TEXT_LAYOUT.md#layout-and-measurement) — Строки, метрики и контуры раскладки. |
| [`fvm_text_layout_destroy`](../../../include/flashvm/text_layout.h#L75) | [TEXT_LAYOUT.md#layout-and-measurement](../../../doc/TEXT_LAYOUT.md#layout-and-measurement) — Строки, метрики и контуры раскладки. |
| [`fvm_text_layout_glyph_path`](../../../include/flashvm/text_layout.h#L94) | [TEXT_LAYOUT.md#layout-and-measurement](../../../doc/TEXT_LAYOUT.md#layout-and-measurement) — Строки, метрики и контуры раскладки. |
| [`fvm_text_layout_line`](../../../include/flashvm/text_layout.h#L86) | [TEXT_LAYOUT.md#layout-and-measurement](../../../doc/TEXT_LAYOUT.md#layout-and-measurement) — Строки, метрики и контуры раскладки. |
| [`fvm_text_layout_metrics`](../../../include/flashvm/text_layout.h#L84) | [TEXT_LAYOUT.md#layout-and-measurement](../../../doc/TEXT_LAYOUT.md#layout-and-measurement) — Строки, метрики и контуры раскладки. |
| [`fvm_text_layout_path`](../../../include/flashvm/text_layout.h#L92) | [TEXT_LAYOUT.md#layout-and-measurement](../../../doc/TEXT_LAYOUT.md#layout-and-measurement) — Строки, метрики и контуры раскладки. |

[Описание в общем каталоге](../../README.md#example-embedding-text_layout).
<!-- example-catalog-api:end -->

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_font_destroy` | function | [include/flashvm/font.h:48](../../../include/flashvm/font.h#L48) |
| `fvm_font_read` | function | [include/flashvm/font.h:45](../../../include/flashvm/font.h#L45) |
| `fvm_path_destroy` | function | [include/flashvm/path.h:52](../../../include/flashvm/path.h#L52) |
| `fvm_path_draw` | function | [include/flashvm/path.h:81](../../../include/flashvm/path.h#L81) |
| `fvm_path_transform` | function | [include/flashvm/path.h:65](../../../include/flashvm/path.h#L65) |
| `fvm_text_layout_create` | function | [include/flashvm/text_layout.h:72](../../../include/flashvm/text_layout.h#L72) |
| `fvm_text_layout_destroy` | function | [include/flashvm/text_layout.h:75](../../../include/flashvm/text_layout.h#L75) |
| `fvm_text_layout_glyph_path` | function | [include/flashvm/text_layout.h:94](../../../include/flashvm/text_layout.h#L94) |
| `fvm_text_layout_line` | function | [include/flashvm/text_layout.h:86](../../../include/flashvm/text_layout.h#L86) |
| `fvm_text_layout_metrics` | function | [include/flashvm/text_layout.h:84](../../../include/flashvm/text_layout.h#L84) |
| `fvm_text_layout_path` | function | [include/flashvm/text_layout.h:92](../../../include/flashvm/text_layout.h#L92) |
<!-- source-links:end -->
