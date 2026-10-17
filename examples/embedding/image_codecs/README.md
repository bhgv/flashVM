# embedding/image_codecs

Разбирает и растеризует SVG и использует подключённую PNG-библиотеку. Демонстрирует обмен буферами изображения и освобождение SVG; доступность зависит от выбранных библиотек и профиля.

## Реализация и запуск

Пример C/C++ собирается с `FVM_BUILD_EXAMPLES=ON`. Из корня репозитория:

```sh
cmake -S . -B build-examples -DFVM_DEFS_FILE=cmake/profiles/agg.defs -DFVM_BUILD_EXAMPLES=ON
cmake --build build-examples --target flashvm_examples_embedding_image_codecs_main --parallel
./build-examples/flashvm_examples_embedding_image_codecs_main
```

Операции зависят от выбранного DEFS-профиля; OS_PORTS, JIT, графика и codecs
не включаются наличием этого README. Исходники ниже показывают порядок API-вызовов
и освобождения ресурсов.

<!-- example-catalog-api:start -->
**Текущая CMake-цель:** `flashvm_examples_embedding_image_codecs_main`.


## Демонстрируемые функции C API

Имена ведут к публичным декларациям. Во второй колонке указан файл руководства
и раздел с контрактом; общие функции setup/cleanup включены для воспроизводимости.

| Функция | Где читать подробнее |
| --- | --- |
| [`fvm_svg_destroy`](../../../include/flashvm/svg.h) | [IMAGE_CODECS.md](../../../doc/IMAGE_CODECS.md) — Разбор SVG и растеризация. |
| [`fvm_svg_rasterize`](../../../include/flashvm/svg.h) | [IMAGE_CODECS.md](../../../doc/IMAGE_CODECS.md) — Разбор SVG и растеризация. |
| [`fvm_svg_read`](../../../include/flashvm/svg.h) | [IMAGE_CODECS.md](../../../doc/IMAGE_CODECS.md) — Разбор SVG и растеризация. |

[Описание в общем каталоге](../../README.md#example-embedding-image_codecs).
<!-- example-catalog-api:end -->

**Профиль сборки:** необходим `IMAGE_CODECS` в DEFS. При отключённой возможности
CMake вообще не создаёт цель этого примера. Профиль `cmake/profiles/agg.defs`
включает её; `minimal.defs` отключает.
