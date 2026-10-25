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
| [`fvm_config_init`](../../../include/flashvm/flashvm.h#L70) | [API_INDEX.md#runtime-and-context](../../../doc/API_INDEX.md#runtime-and-context) — Жизненный цикл runtime/context; точные сигнатуры и ownership — в заголовке. |
| [`fvm_construct`](../../../include/flashvm/value.h#L232) | [API_INDEX.md#values-and-borrowed-views](../../../doc/API_INDEX.md#values-and-borrowed-views) — Типы значений, borrowed-данные и преобразования. |
| [`fvm_context_create`](../../../include/flashvm/flashvm.h#L82) | [API_INDEX.md#runtime-and-context](../../../doc/API_INDEX.md#runtime-and-context) — Жизненный цикл runtime/context; точные сигнатуры и ownership — в заголовке. |
| [`fvm_context_destroy`](../../../include/flashvm/flashvm.h#L86) | [API_INDEX.md#runtime-and-context](../../../doc/API_INDEX.md#runtime-and-context) — Жизненный цикл runtime/context; точные сигнатуры и ownership — в заголовке. |
| [`fvm_gc_pin`](../../../include/flashvm/gc.h#L60) | [GARBAGE_COLLECTION.md#api](../../../doc/GARBAGE_COLLECTION.md#api) — Корни, pin/unpin, режимы и ручная сборка. |
| [`fvm_gc_unpin`](../../../include/flashvm/gc.h#L70) | [GARBAGE_COLLECTION.md#api](../../../doc/GARBAGE_COLLECTION.md#api) — Корни, pin/unpin, режимы и ручная сборка. |
| [`fvm_movieclip_create`](../../../include/flashvm/movieclip.h#L120) | [STANDARD_OBJECTS.md#movieclip-model](../../../doc/STANDARD_OBJECTS.md#movieclip-model) — MovieClip и host API сцены. |
| [`fvm_movieclip_render`](../../../include/flashvm/movieclip.h#L174) | [STANDARD_OBJECTS.md#movieclip-model](../../../doc/STANDARD_OBJECTS.md#movieclip-model) — MovieClip и host API сцены. |
| [`fvm_object_call`](../../../include/flashvm/value.h#L184) | [AVM1_FUNCTIONS.md](../../../doc/AVM1_FUNCTIONS.md) — Метод, receiver и аргументы. |
| [`fvm_runtime_create`](../../../include/flashvm/flashvm.h#L74) | [API_INDEX.md#runtime-and-context](../../../doc/API_INDEX.md#runtime-and-context) — Жизненный цикл runtime/context; точные сигнатуры и ownership — в заголовке. |
| [`fvm_runtime_destroy`](../../../include/flashvm/flashvm.h#L78) | [API_INDEX.md#runtime-and-context](../../../doc/API_INDEX.md#runtime-and-context) — Жизненный цикл runtime/context; точные сигнатуры и ownership — в заголовке. |
| [`fvm_standard_install`](../../../include/flashvm/standard.h#L22) | [STANDARD_OBJECTS.md](../../../doc/STANDARD_OBJECTS.md) — Стандартные классы и объектная модель. |
| [`fvm_value_boolean`](../../../include/flashvm/value.h#L54) | Записывает тег Boolean и нормализует значение: 0 становится false, любое ненулевое число — true. Подробнее: [API_INDEX.md#values-and-borrowed-views](../../../doc/API_INDEX.md#values-and-borrowed-views). |
| [`fvm_value_number`](../../../include/flashvm/value.h#L50) | [API_INDEX.md#values-and-borrowed-views](../../../doc/API_INDEX.md#values-and-borrowed-views) — Типы значений, borrowed-данные и преобразования. |
| [`fvm_value_undefined`](../../../include/flashvm/value.h#L46) | [API_INDEX.md#values-and-borrowed-views](../../../doc/API_INDEX.md#values-and-borrowed-views) — Типы значений, borrowed-данные и преобразования. |

[Описание в общем каталоге](../../README.md#example-embedding-bitmap_fill).
<!-- example-catalog-api:end -->


File-authored image strokes now share the sampler and retained BitmapData ownership:
[SWF8 bitmap LINESTYLE2](../../../doc/BITMAP_STROKES.md). This C89 example still
constructs a script bitmap fill; it does not encode a SWF. The guide links actual
file-loading tests and the remaining stroke limitations.

Файловые bitmap-обводки используют тот же семплер и владение BitmapData:
[SWF8 bitmap LINESTYLE2](../../../doc/BITMAP_STROKES.ru.md). Этот C89-пример строит
скриптовую bitmap-заливку и не кодирует SWF. В руководстве перечислены настоящие
байтовые тесты загрузки и оставшиеся ограничения обводок.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_config_init` | function | [include/flashvm/flashvm.h:70](../../../include/flashvm/flashvm.h#L70) |
| `fvm_construct` | function | [include/flashvm/value.h:232](../../../include/flashvm/value.h#L232) |
| `fvm_context_create` | function | [include/flashvm/flashvm.h:82](../../../include/flashvm/flashvm.h#L82) |
| `fvm_context_destroy` | function | [include/flashvm/flashvm.h:86](../../../include/flashvm/flashvm.h#L86) |
| `fvm_gc_pin` | function | [include/flashvm/gc.h:60](../../../include/flashvm/gc.h#L60) |
| `fvm_gc_unpin` | function | [include/flashvm/gc.h:70](../../../include/flashvm/gc.h#L70) |
| `fvm_movieclip_create` | function | [include/flashvm/movieclip.h:120](../../../include/flashvm/movieclip.h#L120) |
| `fvm_movieclip_render` | function | [include/flashvm/movieclip.h:174](../../../include/flashvm/movieclip.h#L174) |
| `fvm_object_call` | function | [include/flashvm/value.h:184](../../../include/flashvm/value.h#L184) |
| `fvm_runtime_create` | function | [include/flashvm/flashvm.h:74](../../../include/flashvm/flashvm.h#L74) |
| `fvm_runtime_destroy` | function | [include/flashvm/flashvm.h:78](../../../include/flashvm/flashvm.h#L78) |
| `fvm_standard_install` | function | [include/flashvm/standard.h:22](../../../include/flashvm/standard.h#L22) |
| `fvm_value_boolean` | function | [include/flashvm/value.h:54](../../../include/flashvm/value.h#L54) |
| `fvm_value_number` | function | [include/flashvm/value.h:50](../../../include/flashvm/value.h#L50) |
| `fvm_value_undefined` | function | [include/flashvm/value.h:46](../../../include/flashvm/value.h#L46) |
<!-- source-links:end -->
