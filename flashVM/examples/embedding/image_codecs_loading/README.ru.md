# Загрузка изображения через C89

[English](README.md) · [Каталог](../../README.md#example-embedding-image_codecs_loading) ·
[Контракт](../../../doc/IMAGE_LOADING.ru.md).

[main.c](main.c) загружает immutable counted PNG через image и resource API,
делает GC, рисует в host RGBA, проверяет красный пиксель, очищает клип и проверяет
собственное свойство. Не нужны SDL, файл, сеть, SWF или AS-компилятор.
[picture.inc](picture.inc) содержит красный/полупрозрачный зелёный/синий/прозрачный
пиксели; [make_picture.py](make_picture.py) воспроизводит PNG через stdlib.

Команды configure/build/run и генерации — в [English](README.md).
Цель: `flashvm_examples_embedding_image_codecs_loading_main`; код 0 и строка
`image pixels verified; instance data retained`. Нужен player-images.defs с
приложенными статическими codecs. Компонент собирается отдельно через -S . внутри
flashVM. Для другой ОС/CPU выбирается [toolchain](../../../doc/DESKTOP_FRONTEND.ru.md)
и та же цель; SDL SDK самому примеру не нужен. Windows добавляет .exe.
Кросс-линковка не означает запуск; NO_IMAGE_CODECS исключает пример.

Поток владельца, pin удерживает граф через GC. Контекст освобождает pin/bitmap до
runtime. Вход не меняется и не освобождается библиотекой; bitmap владеет копией
pixels. Выход RGBA заимствуется на render. Ошибки идут через обычный cleanup.
Пределы, source profiles, unsupported GIF и ошибки — в IMAGE_LOADING.ru.md.

| API | Назначение и владение; руководство |
| --- | --- |
| [`fvm_movieclip_content_info_init`](../../../include/flashvm/movieclip.h#L72) | Заполнить структуру вызывающего и размер ABI без выделения памяти; [MOVIECLIP_CONTENT_INFO.ru.md](../../../doc/MOVIECLIP_CONTENT_INFO.ru.md). |
| [`fvm_movieclip_get_content_info`](../../../include/flashvm/movieclip.h#L83) | Копировать сведения об источнике/пикселях без скрипта и I/O; ошибка сохраняет результат; [MOVIECLIP_CONTENT_INFO.ru.md](../../../doc/MOVIECLIP_CONTENT_INFO.ru.md). |
| [`fvm_config_init`](../../../include/flashvm/flashvm.h#L70) | Инициализация структуры вызывающего; API_INDEX.md. |
| [`fvm_runtime_create`](../../../include/flashvm/flashvm.h#L74) | Внешний владелец, освобождается последним; API_INDEX.md. |
| [`fvm_context_create`](../../../include/flashvm/flashvm.h#L82) | Владелец managed graph; API_INDEX.md. |
| [`fvm_standard_install`](../../../include/flashvm/standard.h#L22) | Trusted native classes; API_INDEX.md. |
| [`fvm_movieclip_create`](../../../include/flashvm/movieclip.h#L120) | Заимствованный экземпляр; MOVIECLIP_TICKS.md. |
| [`fvm_gc_pin`](../../../include/flashvm/gc.h#L60) | Удержание графа между вызовами; GC.md. |
| [`fvm_value_string_size`](../../../include/flashvm/value.h#L67) | Получить длину заимствованного источника без выделения памяти и передачи владения; API_INDEX.md. |
| [`fvm_value_number`](../../../include/flashvm/value.h#L50) | Scalar без managed storage; API_INDEX.md. |
| [`fvm_object_set`](../../../include/flashvm/value.h#L106) | Копирование свойства; API_INDEX.md. |
| [`fvm_movieclip_replace_image`](../../../include/flashvm/movieclip.h#L235) | Заимствование input/source и атомарная замена; IMAGE_LOADING.ru.md. |
| [`fvm_movieclip_replace_resource`](../../../include/flashvm/movieclip.h#L241) | Dispatch SWF/image по bytes; IMAGE_LOADING.ru.md. |
| [`fvm_gc_collect`](../../../include/flashvm/gc.h#L54) | GC при закреплённом экземпляре; GC.md. |
| [`fvm_movieclip_render`](../../../include/flashvm/movieclip.h#L174) | Синхронно заимствует RGBA output; IMAGE_LOADING.ru.md. |
| [`fvm_movieclip_unload`](../../../include/flashvm/movieclip.h#L264) | Удаляет content, сохраняет identity/data; MOVIE_UNLOAD.ru.md. |
| [`fvm_object_get`](../../../include/flashvm/value.h#L102) | Чтение сохранённого scalar; API_INDEX.md. |
| [`fvm_context_destroy`](../../../include/flashvm/flashvm.h#L86) | Освобождает graph/pin; API_INDEX.md. |
| [`fvm_runtime_destroy`](../../../include/flashvm/flashvm.h#L78) | Освобождает runtime после context; API_INDEX.md. |


C89-пример также читает и меняет `_width` через [fvm_object_get](../../../include/flashvm/value.h#L102)/set, восстанавливает
геометрию перед сравнением пикселей и читает источник `_url` с явной длиной.
Правила преобразования, времени жизни и переноса описаны в
[руководстве свойств](../../../doc/MOVIECLIP_NAMED_PROPERTIES.ru.md).


С v0.709 пример инициализирует [FVM_MovieClipContentInfo](../../../include/flashvm/movieclip.h#L59), сверяет длину PNG с исходным
RGBA-буфером и размерами, после выгрузки требует явную запись EMPTY. Снимок не
содержит заимствованных указателей и хранится независимо от графа. Это не измерение
текущего прогресса передачи.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_config_init` | function | [include/flashvm/flashvm.h:70](../../../include/flashvm/flashvm.h#L70) |
| `fvm_context_create` | function | [include/flashvm/flashvm.h:82](../../../include/flashvm/flashvm.h#L82) |
| `fvm_context_destroy` | function | [include/flashvm/flashvm.h:86](../../../include/flashvm/flashvm.h#L86) |
| `fvm_gc_collect` | function | [include/flashvm/gc.h:54](../../../include/flashvm/gc.h#L54) |
| `fvm_gc_pin` | function | [include/flashvm/gc.h:60](../../../include/flashvm/gc.h#L60) |
| `fvm_movieclip_content_info_init` | function | [include/flashvm/movieclip.h:72](../../../include/flashvm/movieclip.h#L72) |
| `fvm_movieclip_create` | function | [include/flashvm/movieclip.h:120](../../../include/flashvm/movieclip.h#L120) |
| `fvm_movieclip_get_content_info` | function | [include/flashvm/movieclip.h:83](../../../include/flashvm/movieclip.h#L83) |
| `fvm_movieclip_render` | function | [include/flashvm/movieclip.h:174](../../../include/flashvm/movieclip.h#L174) |
| `fvm_movieclip_replace_image` | function | [include/flashvm/movieclip.h:235](../../../include/flashvm/movieclip.h#L235) |
| `fvm_movieclip_replace_resource` | function | [include/flashvm/movieclip.h:241](../../../include/flashvm/movieclip.h#L241) |
| `fvm_movieclip_unload` | function | [include/flashvm/movieclip.h:264](../../../include/flashvm/movieclip.h#L264) |
| `FVM_MovieClipContentInfo` | struct | [include/flashvm/movieclip.h:59](../../../include/flashvm/movieclip.h#L59) |
| `fvm_object_get` | function | [include/flashvm/value.h:102](../../../include/flashvm/value.h#L102) |
| `fvm_object_set` | function | [include/flashvm/value.h:106](../../../include/flashvm/value.h#L106) |
| `fvm_runtime_create` | function | [include/flashvm/flashvm.h:74](../../../include/flashvm/flashvm.h#L74) |
| `fvm_runtime_destroy` | function | [include/flashvm/flashvm.h:78](../../../include/flashvm/flashvm.h#L78) |
| `fvm_standard_install` | function | [include/flashvm/standard.h:22](../../../include/flashvm/standard.h#L22) |
| `fvm_value_number` | function | [include/flashvm/value.h:50](../../../include/flashvm/value.h#L50) |
| `fvm_value_string_size` | function | [include/flashvm/value.h:67](../../../include/flashvm/value.h#L67) |
<!-- source-links:end -->
