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
| [`fvm_font_destroy`](../../../include/flashvm/font.h) | [FONT_LIBRARIES.md#flashvm-c89-embedding-api](../../../doc/FONT_LIBRARIES.md#flashvm-c89-embedding-api) — Шрифты, shaping и редактируемые контуры. |
| [`fvm_font_read`](../../../include/flashvm/font.h) | [FONT_LIBRARIES.md#flashvm-c89-embedding-api](../../../doc/FONT_LIBRARIES.md#flashvm-c89-embedding-api) — Шрифты, shaping и редактируемые контуры. |
| [`fvm_font_shape`](../../../include/flashvm/font.h) | [FONT_LIBRARIES.md#flashvm-c89-embedding-api](../../../doc/FONT_LIBRARIES.md#flashvm-c89-embedding-api) — Шрифты, shaping и редактируемые контуры. |
| [`fvm_path_bounds`](../../../include/flashvm/path.h) | [FONT_LIBRARIES.md#owned-vector-model](../../../doc/FONT_LIBRARIES.md#owned-vector-model) — Владеющие векторные пути и преобразования. |
| [`fvm_path_destroy`](../../../include/flashvm/path.h) | [FONT_LIBRARIES.md#owned-vector-model](../../../doc/FONT_LIBRARIES.md#owned-vector-model) — Владеющие векторные пути и преобразования. |
| [`fvm_path_draw`](../../../include/flashvm/path.h) | [FONT_LIBRARIES.md#owned-vector-model](../../../doc/FONT_LIBRARIES.md#owned-vector-model) — Владеющие векторные пути и преобразования. |
| [`fvm_path_get`](../../../include/flashvm/path.h) | [FONT_LIBRARIES.md#owned-vector-model](../../../doc/FONT_LIBRARIES.md#owned-vector-model) — Владеющие векторные пути и преобразования. |
| [`fvm_path_mask`](../../../include/flashvm/path.h) | [FONT_LIBRARIES.md#owned-vector-model](../../../doc/FONT_LIBRARIES.md#owned-vector-model) — Владеющие векторные пути и преобразования. |
| [`fvm_path_set_point`](../../../include/flashvm/path.h) | [FONT_LIBRARIES.md#owned-vector-model](../../../doc/FONT_LIBRARIES.md#owned-vector-model) — Владеющие векторные пути и преобразования. |
| [`fvm_path_transform`](../../../include/flashvm/path.h) | [FONT_LIBRARIES.md#owned-vector-model](../../../doc/FONT_LIBRARIES.md#owned-vector-model) — Владеющие векторные пути и преобразования. |
| [`fvm_text_run_count`](../../../include/flashvm/font.h) | [FONT_LIBRARIES.md#flashvm-c89-embedding-api](../../../doc/FONT_LIBRARIES.md#flashvm-c89-embedding-api) — Шрифты, shaping и редактируемые контуры. |
| [`fvm_text_run_destroy`](../../../include/flashvm/font.h) | [FONT_LIBRARIES.md#flashvm-c89-embedding-api](../../../doc/FONT_LIBRARIES.md#flashvm-c89-embedding-api) — Шрифты, shaping и редактируемые контуры. |
| [`fvm_text_run_path`](../../../include/flashvm/font.h) | [FONT_LIBRARIES.md#flashvm-c89-embedding-api](../../../doc/FONT_LIBRARIES.md#flashvm-c89-embedding-api) — Шрифты, shaping и редактируемые контуры. |

[Описание в общем каталоге](../../README.md#example-embedding-font_libraries).
<!-- example-catalog-api:end -->

**Профиль сборки:** необходим `FONTS` в DEFS. При отключённой возможности
CMake вообще не создаёт цель этого примера. Профиль `cmake/profiles/agg.defs`
включает её; `minimal.defs` отключает.
