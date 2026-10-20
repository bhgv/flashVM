# Качество сцены / Stage quality

Пример показывает реальный эффект AVM1 ToggleQuality на дробных краях одного
красного треугольника. Сначала рисуется HIGH, затем Action 0x08 переводит
контекст в LOW. Программа печатает количество пикселей с промежуточной альфой:
в HIGH оно положительно, в LOW — ноль. Точное число зависит от AGG/Cairo.

Пример также устанавливает `_quality` через обычное свойство MovieClip и читает
числовое `_highquality`; обе формы обращаются к состоянию всего контекста.

## Реализация и владение

Хост владеет маленьким RGBA-буфером; render использует его только синхронно.
MovieClip и его drawing state принадлежат контексту. Явный pin удерживает clip
через исполнение Action и любые GC safe points; cleanup снимает корень и
уничтожает контекст раньше runtime. После unpin указатель больше не читается.

Качество хранится в контексте, а не в отдельном clip. Вызов standalone path API
без контекста не использует эту настройку. LOW меняет покрытие края, а не
прозрачность краски или smoothing изображения. BEST сохраняет предпочтение при
ToggleQuality, но дополнительные bitmap sampling grids пока не реализованы.

Каждая непустая строка кода прокомментирована. Ошибки обрабатываются обычными
ветками и cleanup, без assert. Файлы и сетевые соединения не создаются.

## Сборка и запуск

Из корня проекта:

```sh
cmake -S . -B build-quality -DFVM_DEFS_FILE=cmake/profiles/agg.defs -DFVM_BUILD_EXAMPLES=ON -DCMAKE_BUILD_TYPE=Release
cmake --build build-quality --target flashvm_examples_embedding_stage_quality_main --parallel
./build-quality/flashvm_examples_embedding_stage_quality_main
```

Для Cairo выберите `cmake/profiles/cairo.defs`. В minimal/NO_GRAPHICS программа
объясняет, какой backend включить, и не заявляет, что выполнила растеризацию.

С v0.557 та же операция поддержана в compiled function bodies через прямой
JIT-helper; этот пример по-прежнему вызывает root [fvm_avm1_execute](../../../include/flashvm/avm1.h#L38). Проверка
трёх режимов и actual native entries описана в
[STAGE_QUALITY.md](../../../doc/STAGE_QUALITY.md#native-verification--проверки-jit-v0557).
Это не новая проверка AGG/Cairo или иной ОС.

## Демонстрируемые функции API

- [`fvm_value_string`](../../../include/flashvm/value.h#L46) — подготовка строкового аргумента качества: [STAGE_QUALITY.md](../../../doc/STAGE_QUALITY.md#script-quality-properties).

- [`fvm_object_set`](../../../include/flashvm/value.h#L74), [`fvm_object_get`](../../../include/flashvm/value.h#L72) — запись `_quality` и чтение `_highquality`: [свойства и преобразования](../../../doc/STAGE_QUALITY.md#script-quality-properties).


- [`fvm_stage_set_quality`](../../../include/flashvm/stage.h#L22), [`fvm_stage_get_quality`](../../../include/flashvm/stage.h#L19) — настройка и чтение политики: [STAGE_QUALITY.md](../../../doc/STAGE_QUALITY.md#fvm_stage_set_quality), [getter](../../../doc/STAGE_QUALITY.md#fvm_stage_get_quality).
- [`fvm_avm1_execute`](../../../include/flashvm/avm1.h#L38) — настоящий ToggleQuality и переход к LOW: [контракт действия](../../../doc/STAGE_QUALITY.md#contract).
- [`fvm_movieclip_create`](../../../include/flashvm/movieclip.h#L69), [`fvm_movieclip_render`](../../../include/flashvm/movieclip.h#L109) — host clip и RGBA-рендер: [renderer integration](../../../doc/STAGE_QUALITY.md#renderer-integration-and-limits).
- [`fvm_standard_install`](../../../include/flashvm/standard.h#L20), [`fvm_object_call`](../../../include/flashvm/value.h#L126), [`fvm_value_number`](../../../include/flashvm/value.h#L42) — drawing API через обычные native-методы: [STANDARD_OBJECTS.md](../../../doc/STANDARD_OBJECTS.md#movieclip-model).
- [`fvm_config_init`](../../../include/flashvm/flashvm.h#L62), [`fvm_runtime_create`](../../../include/flashvm/flashvm.h#L64), [`fvm_context_create`](../../../include/flashvm/flashvm.h#L68), [`fvm_context_destroy`](../../../include/flashvm/flashvm.h#L70), [`fvm_runtime_destroy`](../../../include/flashvm/flashvm.h#L66) — ownership и порядок очистки: [API_INDEX.md](../../../doc/API_INDEX.md#runtime-and-context).

- [`fvm_gc_pin`](../../../include/flashvm/gc.h#L48), [`fvm_gc_unpin`](../../../include/flashvm/gc.h#L54) — явный корень удерживаемого C-хостом clip: [GC API](../../../doc/GARBAGE_COLLECTION.md#api).

## English

A line-commented C89 tutorial renders a fractional triangle at HIGH, executes
AVM1 ToggleQuality and renders LOW. It compares partial-alpha edge counts, with
ordinary error cleanup and no assertions. AGG/Cairo are selected explicitly;
NO_GRAPHICS is reported as a disabled backend. Context quality controls vector
coverage; independent path APIs and bitmap smoothing keep their own contracts.

Since v0.557 compiled function bodies also lower ToggleQuality directly. This
example continues to use root [fvm_avm1_execute](../../../include/flashvm/avm1.h#L38); native-entry evidence comes from
the linked three-mode tests, not from a new renderer/platform run.

[Каталог](../../README.md#example-embedding-stage_quality).

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_avm1_execute` | function | [include/flashvm/avm1.h:38](../../../include/flashvm/avm1.h#L38) |
| `fvm_config_init` | function | [include/flashvm/flashvm.h:62](../../../include/flashvm/flashvm.h#L62) |
| `fvm_context_create` | function | [include/flashvm/flashvm.h:68](../../../include/flashvm/flashvm.h#L68) |
| `fvm_context_destroy` | function | [include/flashvm/flashvm.h:70](../../../include/flashvm/flashvm.h#L70) |
| `fvm_gc_pin` | function | [include/flashvm/gc.h:48](../../../include/flashvm/gc.h#L48) |
| `fvm_gc_unpin` | function | [include/flashvm/gc.h:54](../../../include/flashvm/gc.h#L54) |
| `fvm_movieclip_create` | function | [include/flashvm/movieclip.h:69](../../../include/flashvm/movieclip.h#L69) |
| `fvm_movieclip_render` | function | [include/flashvm/movieclip.h:109](../../../include/flashvm/movieclip.h#L109) |
| `fvm_object_call` | function | [include/flashvm/value.h:126](../../../include/flashvm/value.h#L126) |
| `fvm_object_get` | function | [include/flashvm/value.h:72](../../../include/flashvm/value.h#L72) |
| `fvm_object_set` | function | [include/flashvm/value.h:74](../../../include/flashvm/value.h#L74) |
| `fvm_runtime_create` | function | [include/flashvm/flashvm.h:64](../../../include/flashvm/flashvm.h#L64) |
| `fvm_runtime_destroy` | function | [include/flashvm/flashvm.h:66](../../../include/flashvm/flashvm.h#L66) |
| `fvm_stage_get_quality` | function | [include/flashvm/stage.h:19](../../../include/flashvm/stage.h#L19) |
| `fvm_stage_set_quality` | function | [include/flashvm/stage.h:22](../../../include/flashvm/stage.h#L22) |
| `fvm_standard_install` | function | [include/flashvm/standard.h:20](../../../include/flashvm/standard.h#L20) |
| `fvm_value_number` | function | [include/flashvm/value.h:42](../../../include/flashvm/value.h#L42) |
| `fvm_value_string` | function | [include/flashvm/value.h:46](../../../include/flashvm/value.h#L46) |
<!-- source-links:end -->
