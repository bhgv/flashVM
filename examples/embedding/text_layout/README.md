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
| [`fvm_font_destroy`](../../../include/flashvm/font.h) | [FONT_LIBRARIES.md#flashvm-c89-embedding-api](../../../doc/FONT_LIBRARIES.md#flashvm-c89-embedding-api) — Шрифты, shaping и редактируемые контуры. |
| [`fvm_font_read`](../../../include/flashvm/font.h) | [FONT_LIBRARIES.md#flashvm-c89-embedding-api](../../../doc/FONT_LIBRARIES.md#flashvm-c89-embedding-api) — Шрифты, shaping и редактируемые контуры. |
| [`fvm_path_destroy`](../../../include/flashvm/path.h) | [FONT_LIBRARIES.md#owned-vector-model](../../../doc/FONT_LIBRARIES.md#owned-vector-model) — Владеющие векторные пути и преобразования. |
| [`fvm_path_draw`](../../../include/flashvm/path.h) | [FONT_LIBRARIES.md#owned-vector-model](../../../doc/FONT_LIBRARIES.md#owned-vector-model) — Владеющие векторные пути и преобразования. |
| [`fvm_path_transform`](../../../include/flashvm/path.h) | [FONT_LIBRARIES.md#owned-vector-model](../../../doc/FONT_LIBRARIES.md#owned-vector-model) — Владеющие векторные пути и преобразования. |
| [`fvm_text_layout_create`](../../../include/flashvm/text_layout.h) | [TEXT_LAYOUT.md#layout-and-measurement](../../../doc/TEXT_LAYOUT.md#layout-and-measurement) — Строки, метрики и контуры раскладки. |
| [`fvm_text_layout_destroy`](../../../include/flashvm/text_layout.h) | [TEXT_LAYOUT.md#layout-and-measurement](../../../doc/TEXT_LAYOUT.md#layout-and-measurement) — Строки, метрики и контуры раскладки. |
| [`fvm_text_layout_glyph_path`](../../../include/flashvm/text_layout.h) | [TEXT_LAYOUT.md#layout-and-measurement](../../../doc/TEXT_LAYOUT.md#layout-and-measurement) — Строки, метрики и контуры раскладки. |
| [`fvm_text_layout_line`](../../../include/flashvm/text_layout.h) | [TEXT_LAYOUT.md#layout-and-measurement](../../../doc/TEXT_LAYOUT.md#layout-and-measurement) — Строки, метрики и контуры раскладки. |
| [`fvm_text_layout_metrics`](../../../include/flashvm/text_layout.h) | [TEXT_LAYOUT.md#layout-and-measurement](../../../doc/TEXT_LAYOUT.md#layout-and-measurement) — Строки, метрики и контуры раскладки. |
| [`fvm_text_layout_path`](../../../include/flashvm/text_layout.h) | [TEXT_LAYOUT.md#layout-and-measurement](../../../doc/TEXT_LAYOUT.md#layout-and-measurement) — Строки, метрики и контуры раскладки. |

[Описание в общем каталоге](../../README.md#example-embedding-text_layout).
<!-- example-catalog-api:end -->
