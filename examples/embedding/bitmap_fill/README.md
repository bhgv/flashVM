# embedding/bitmap_fill

Создаёт BitmapData из двух пикселей, использует живой bitmap как заливку MovieClip и рендерит результат. Показывает сохранение GC-корней для удерживаемых объектов и порядок освобождения ресурсов.

## Реализация и запуск

Пример C/C++ собирается с `FVM_BUILD_EXAMPLES=ON`. Из корня репозитория:

```sh
cmake -S . -B build-examples -DFVM_DEFS_FILE=cmake/profiles/minimal.defs -DFVM_BUILD_EXAMPLES=ON
cmake --build build-examples --target flashvm_examples_embedding_bitmap_fill_main --parallel
./build-examples/flashvm_examples_embedding_bitmap_fill_main
```

Операции зависят от выбранного DEFS-профиля; OS_PORTS, JIT, графика и codecs
не включаются наличием этого README. Исходники ниже показывают порядок API-вызовов
и освобождения ресурсов.

<!-- example-catalog-api:start -->
**Текущая CMake-цель:** `flashvm_examples_embedding_bitmap_fill_main`.


## Демонстрируемые функции C API

Имена ведут к публичным декларациям. Во второй колонке описано действие каждой
функции и дан переход к руководству; общие функции setup/cleanup включены для воспроизводимости.

| Функция | Назначение и документация |
| --- | --- |
| [`fvm_config_init`](../../../include/flashvm/flashvm.h#L62) | [API_INDEX.md#runtime-and-context](../../../doc/API_INDEX.md#runtime-and-context) — Жизненный цикл runtime/context; точные сигнатуры и ownership — в заголовке. |
| [`fvm_construct`](../../../include/flashvm/value.h#L158) | [API_INDEX.md#values-and-borrowed-views](../../../doc/API_INDEX.md#values-and-borrowed-views) — Типы значений, borrowed-данные и преобразования. |
| [`fvm_context_create`](../../../include/flashvm/flashvm.h#L68) | [API_INDEX.md#runtime-and-context](../../../doc/API_INDEX.md#runtime-and-context) — Жизненный цикл runtime/context; точные сигнатуры и ownership — в заголовке. |
| [`fvm_context_destroy`](../../../include/flashvm/flashvm.h#L70) | [API_INDEX.md#runtime-and-context](../../../doc/API_INDEX.md#runtime-and-context) — Жизненный цикл runtime/context; точные сигнатуры и ownership — в заголовке. |
| [`fvm_gc_pin`](../../../include/flashvm/gc.h#L48) | [GARBAGE_COLLECTION.md#api](../../../doc/GARBAGE_COLLECTION.md#api) — Корни, pin/unpin, режимы и ручная сборка. |
| [`fvm_gc_unpin`](../../../include/flashvm/gc.h#L54) | [GARBAGE_COLLECTION.md#api](../../../doc/GARBAGE_COLLECTION.md#api) — Корни, pin/unpin, режимы и ручная сборка. |
| [`fvm_movieclip_create`](../../../include/flashvm/movieclip.h#L28) | [STANDARD_OBJECTS.md#movieclip-model](../../../doc/STANDARD_OBJECTS.md#movieclip-model) — MovieClip и host API сцены. |
| [`fvm_movieclip_render`](../../../include/flashvm/movieclip.h#L48) | [STANDARD_OBJECTS.md#movieclip-model](../../../doc/STANDARD_OBJECTS.md#movieclip-model) — MovieClip и host API сцены. |
| [`fvm_object_call`](../../../include/flashvm/value.h#L126) | [AVM1_FUNCTIONS.md](../../../doc/AVM1_FUNCTIONS.md) — Метод, receiver и аргументы. |
| [`fvm_runtime_create`](../../../include/flashvm/flashvm.h#L64) | [API_INDEX.md#runtime-and-context](../../../doc/API_INDEX.md#runtime-and-context) — Жизненный цикл runtime/context; точные сигнатуры и ownership — в заголовке. |
| [`fvm_runtime_destroy`](../../../include/flashvm/flashvm.h#L66) | [API_INDEX.md#runtime-and-context](../../../doc/API_INDEX.md#runtime-and-context) — Жизненный цикл runtime/context; точные сигнатуры и ownership — в заголовке. |
| [`fvm_standard_install`](../../../include/flashvm/standard.h#L18) | [STANDARD_OBJECTS.md](../../../doc/STANDARD_OBJECTS.md) — Стандартные классы и объектная модель. |
| [`fvm_value_boolean`](../../../include/flashvm/value.h#L44) | Записывает тег Boolean и нормализует значение: 0 становится false, любое ненулевое число — true. Подробнее: [API_INDEX.md#values-and-borrowed-views](../../../doc/API_INDEX.md#values-and-borrowed-views). |
| [`fvm_value_number`](../../../include/flashvm/value.h#L42) | [API_INDEX.md#values-and-borrowed-views](../../../doc/API_INDEX.md#values-and-borrowed-views) — Типы значений, borrowed-данные и преобразования. |
| [`fvm_value_undefined`](../../../include/flashvm/value.h#L40) | [API_INDEX.md#values-and-borrowed-views](../../../doc/API_INDEX.md#values-and-borrowed-views) — Типы значений, borrowed-данные и преобразования. |

[Описание в общем каталоге](../../README.md#example-embedding-bitmap_fill).
<!-- example-catalog-api:end -->

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_config_init` | function | [include/flashvm/flashvm.h:62](../../../include/flashvm/flashvm.h#L62) |
| `fvm_construct` | function | [include/flashvm/value.h:158](../../../include/flashvm/value.h#L158) |
| `fvm_context_create` | function | [include/flashvm/flashvm.h:68](../../../include/flashvm/flashvm.h#L68) |
| `fvm_context_destroy` | function | [include/flashvm/flashvm.h:70](../../../include/flashvm/flashvm.h#L70) |
| `fvm_gc_pin` | function | [include/flashvm/gc.h:48](../../../include/flashvm/gc.h#L48) |
| `fvm_gc_unpin` | function | [include/flashvm/gc.h:54](../../../include/flashvm/gc.h#L54) |
| `fvm_movieclip_create` | function | [include/flashvm/movieclip.h:28](../../../include/flashvm/movieclip.h#L28) |
| `fvm_movieclip_render` | function | [include/flashvm/movieclip.h:48](../../../include/flashvm/movieclip.h#L48) |
| `fvm_object_call` | function | [include/flashvm/value.h:126](../../../include/flashvm/value.h#L126) |
| `fvm_runtime_create` | function | [include/flashvm/flashvm.h:64](../../../include/flashvm/flashvm.h#L64) |
| `fvm_runtime_destroy` | function | [include/flashvm/flashvm.h:66](../../../include/flashvm/flashvm.h#L66) |
| `fvm_standard_install` | function | [include/flashvm/standard.h:18](../../../include/flashvm/standard.h#L18) |
| `fvm_value_boolean` | function | [include/flashvm/value.h:44](../../../include/flashvm/value.h#L44) |
| `fvm_value_number` | function | [include/flashvm/value.h:42](../../../include/flashvm/value.h#L42) |
| `fvm_value_undefined` | function | [include/flashvm/value.h:40](../../../include/flashvm/value.h#L40) |
<!-- source-links:end -->
