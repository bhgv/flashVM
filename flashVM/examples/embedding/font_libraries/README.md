# embedding/font_libraries

Читает OpenType fixture, формирует text run, извлекает редактируемые векторные контуры и преобразует их. Показывает границы, правку точек, заливку и маску; fixture позволяет не зависеть от установленных шрифтов.

## Реализация и запуск

Пример C/C++ собирается с `FVM_BUILD_EXAMPLES=ON`. Из корня репозитория:

```sh
cmake -S . -B build-examples -DFVM_DEFS_FILE=cmake/profiles/agg.defs -DFVM_BUILD_EXAMPLES=ON
cmake --build build-examples --target flashvm_examples_embedding_font_libraries_main --parallel
./build-examples/flashvm_examples_embedding_font_libraries_main
```

Операции зависят от выбранного DEFS-профиля; OS_PORTS, JIT, графика и codecs
не включаются наличием этого README. Исходники ниже показывают порядок API-вызовов
и освобождения ресурсов.

<!-- example-catalog-api:start -->
**Текущая CMake-цель:** `flashvm_examples_embedding_font_libraries_main`.


## Демонстрируемые функции C API

Имена ведут к публичным декларациям. Во второй колонке указан файл руководства
и раздел с контрактом; общие функции setup/cleanup включены для воспроизводимости.

| Функция | Где читать подробнее |
| --- | --- |
| [`fvm_font_destroy`](../../../include/flashvm/font.h#L69) | [FONT_LIBRARIES.md#flashvm-c89-embedding-api](../../../doc/FONT_LIBRARIES.md#flashvm-c89-embedding-api) — Шрифты, shaping и редактируемые контуры. |
| [`fvm_font_read`](../../../include/flashvm/font.h#L64) | [FONT_LIBRARIES.md#flashvm-c89-embedding-api](../../../doc/FONT_LIBRARIES.md#flashvm-c89-embedding-api) — Шрифты, shaping и редактируемые контуры. |
| [`fvm_font_shape`](../../../include/flashvm/font.h#L107) | [FONT_LIBRARIES.md#flashvm-c89-embedding-api](../../../doc/FONT_LIBRARIES.md#flashvm-c89-embedding-api) — Шрифты, shaping и редактируемые контуры. |
| [`fvm_path_bounds`](../../../include/flashvm/path.h#L97) | [FONT_LIBRARIES.md#owned-vector-model](../../../doc/FONT_LIBRARIES.md#owned-vector-model) — Владеющие векторные пути и преобразования. |
| [`fvm_path_destroy`](../../../include/flashvm/path.h#L64) | [FONT_LIBRARIES.md#owned-vector-model](../../../doc/FONT_LIBRARIES.md#owned-vector-model) — Владеющие векторные пути и преобразования. |
| [`fvm_path_draw`](../../../include/flashvm/path.h#L111) | [FONT_LIBRARIES.md#owned-vector-model](../../../doc/FONT_LIBRARIES.md#owned-vector-model) — Владеющие векторные пути и преобразования. |
| [`fvm_path_get`](../../../include/flashvm/path.h#L76) | [FONT_LIBRARIES.md#owned-vector-model](../../../doc/FONT_LIBRARIES.md#owned-vector-model) — Владеющие векторные пути и преобразования. |
| [`fvm_path_mask`](../../../include/flashvm/path.h#L118) | [FONT_LIBRARIES.md#owned-vector-model](../../../doc/FONT_LIBRARIES.md#owned-vector-model) — Владеющие векторные пути и преобразования. |
| [`fvm_path_set_point`](../../../include/flashvm/path.h#L80) | [FONT_LIBRARIES.md#owned-vector-model](../../../doc/FONT_LIBRARIES.md#owned-vector-model) — Владеющие векторные пути и преобразования. |
| [`fvm_path_transform`](../../../include/flashvm/path.h#L87) | [FONT_LIBRARIES.md#owned-vector-model](../../../doc/FONT_LIBRARIES.md#owned-vector-model) — Владеющие векторные пути и преобразования. |
| [`fvm_text_run_count`](../../../include/flashvm/font.h#L113) | [FONT_LIBRARIES.md#flashvm-c89-embedding-api](../../../doc/FONT_LIBRARIES.md#flashvm-c89-embedding-api) — Шрифты, shaping и редактируемые контуры. |
| [`fvm_text_run_destroy`](../../../include/flashvm/font.h#L133) | [FONT_LIBRARIES.md#flashvm-c89-embedding-api](../../../doc/FONT_LIBRARIES.md#flashvm-c89-embedding-api) — Шрифты, shaping и редактируемые контуры. |
| [`fvm_text_run_path`](../../../include/flashvm/font.h#L139) | [FONT_LIBRARIES.md#flashvm-c89-embedding-api](../../../doc/FONT_LIBRARIES.md#flashvm-c89-embedding-api) — Шрифты, shaping и редактируемые контуры. |

[Описание в общем каталоге](../../README.md#example-embedding-font_libraries).
<!-- example-catalog-api:end -->

**Профиль сборки:** необходим [`FONTS`](../../../cmake/BuildDefinitions.cmake#L15) в DEFS. При отключённой возможности
CMake вообще не создаёт цель этого примера. Профиль `cmake/profiles/agg.defs`
включает её; `minimal.defs` отключает.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FONTS` | DEFS switch | [cmake/BuildDefinitions.cmake:15](../../../cmake/BuildDefinitions.cmake#L15) |
| `fvm_font_destroy` | function | [include/flashvm/font.h:69](../../../include/flashvm/font.h#L69) |
| `fvm_font_read` | function | [include/flashvm/font.h:64](../../../include/flashvm/font.h#L64) |
| `fvm_font_shape` | function | [include/flashvm/font.h:107](../../../include/flashvm/font.h#L107) |
| `fvm_path_bounds` | function | [include/flashvm/path.h:97](../../../include/flashvm/path.h#L97) |
| `fvm_path_destroy` | function | [include/flashvm/path.h:64](../../../include/flashvm/path.h#L64) |
| `fvm_path_draw` | function | [include/flashvm/path.h:111](../../../include/flashvm/path.h#L111) |
| `fvm_path_get` | function | [include/flashvm/path.h:76](../../../include/flashvm/path.h#L76) |
| `fvm_path_mask` | function | [include/flashvm/path.h:118](../../../include/flashvm/path.h#L118) |
| `fvm_path_set_point` | function | [include/flashvm/path.h:80](../../../include/flashvm/path.h#L80) |
| `fvm_path_transform` | function | [include/flashvm/path.h:87](../../../include/flashvm/path.h#L87) |
| `fvm_text_run_count` | function | [include/flashvm/font.h:113](../../../include/flashvm/font.h#L113) |
| `fvm_text_run_destroy` | function | [include/flashvm/font.h:133](../../../include/flashvm/font.h#L133) |
| `fvm_text_run_path` | function | [include/flashvm/font.h:139](../../../include/flashvm/font.h#L139) |
<!-- source-links:end -->
