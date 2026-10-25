# Worker + channel / Поток + канал

A worker runs in a separate VM context. It receives an owned channel handle
through `userdata`, blocks until a number arrives, then returns number + 1.
The caller joins the worker before releasing its retained channel handle.

Рабочий поток запускается с отдельным контекстом VM. Через `userdata` ему
передаётся дескриптор канала. Он ожидает число и возвращает число + 1.
Вызывающая сторона ожидает завершения потока до освобождения дескриптора.

This is a native C89 example, **not** an ActionScript `thread.run` implementation.
Это пример C89, **не** реализация `thread.run` в ActionScript.

<!-- example-catalog-api:start -->
## Назначение и особенности

Передаёт worker через userdata удерживаемый channel handle, ожидает число и возвращает число плюс один. Показывает явное копирование/release дескриптора и join перед освобождением канала.

**Текущая CMake-цель:** `flashvm_examples_extending_channel_worker_main`.


## Демонстрируемые функции C API

Имена ведут к публичным декларациям. Во второй колонке описано действие каждой
функции и дан переход к руководству; общие функции setup/cleanup включены для воспроизводимости.

| Функция | Назначение и документация |
| --- | --- |
| [`fvm_channel_value_copy`](../../../include/flashvm/channel_value.h#L32) | Удерживает дополнительную ссылку на тот же канал; очередь и сообщения не копируются. Каждую успешную копию нужно освободить. Подробнее: [CHANNEL_VALUES.md#fvm_channel_value_copy](../../../doc/CHANNEL_VALUES.md#fvm_channel_value_copy). |
| [`fvm_channel_value_create`](../../../include/flashvm/channel_value.h#L24) | Создаёт новый внутрипроцессный канал с одной владеющей ссылкой; сохраняет дескриптор в out. Подробнее: [CHANNEL_VALUES.md#fvm_channel_value_create](../../../doc/CHANNEL_VALUES.md#fvm_channel_value_create). |
| [`fvm_channel_value_receive`](../../../include/flashvm/channel_value.h#L46) | Получает примитивное сообщение; ждёт при пустой очереди. Полученная строка заимствована у канала. Подробнее: [CHANNEL_VALUES.md#fvm_channel_value_receive](../../../doc/CHANNEL_VALUES.md#fvm_channel_value_receive). |
| [`fvm_channel_value_release`](../../../include/flashvm/channel_value.h#L37) | Освобождает одну ссылку и обнуляет дескриптор; последний release уничтожает канал, а не просто закрывает его. Подробнее: [CHANNEL_VALUES.md#fvm_channel_value_release](../../../doc/CHANNEL_VALUES.md#fvm_channel_value_release). |
| [`fvm_channel_value_send`](../../../include/flashvm/channel_value.h#L41) | Копирует примитивное сообщение в общий канал; ждёт при заполненной ограниченной очереди. Исходное значение остаётся у вызывающего. Подробнее: [CHANNEL_VALUES.md#fvm_channel_value_send](../../../doc/CHANNEL_VALUES.md#fvm_channel_value_send). |
| [`fvm_config_init`](../../../include/flashvm/flashvm.h#L70) | [API_INDEX.md#runtime-and-context](../../../doc/API_INDEX.md#runtime-and-context) — Жизненный цикл runtime/context; точные сигнатуры и ownership — в заголовке. |
| [`fvm_value_number`](../../../include/flashvm/value.h#L50) | [API_INDEX.md#values-and-borrowed-views](../../../doc/API_INDEX.md#values-and-borrowed-views) — Типы значений, borrowed-данные и преобразования. |
| [`fvm_value_undefined`](../../../include/flashvm/value.h#L46) | [API_INDEX.md#values-and-borrowed-views](../../../doc/API_INDEX.md#values-and-borrowed-views) — Типы значений, borrowed-данные и преобразования. |
| [`fvm_worker_destroy`](../../../include/flashvm/worker.h#L98) | [WORKERS.md](../../../doc/WORKERS.md) — Изолированные worker-контексты и join. |
| [`fvm_worker_join`](../../../include/flashvm/worker.h#L60) | [WORKERS.md](../../../doc/WORKERS.md) — Изолированные worker-контексты и join. |
| [`fvm_worker_run`](../../../include/flashvm/worker.h#L25) | [WORKERS.md](../../../doc/WORKERS.md) — Изолированные worker-контексты и join. |

[Описание в общем каталоге](../../README.md#example-extending-channel_worker).
<!-- example-catalog-api:end -->

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_channel_value_copy` | function | [include/flashvm/channel_value.h:32](../../../include/flashvm/channel_value.h#L32) |
| `fvm_channel_value_create` | function | [include/flashvm/channel_value.h:24](../../../include/flashvm/channel_value.h#L24) |
| `fvm_channel_value_receive` | function | [include/flashvm/channel_value.h:46](../../../include/flashvm/channel_value.h#L46) |
| `fvm_channel_value_release` | function | [include/flashvm/channel_value.h:37](../../../include/flashvm/channel_value.h#L37) |
| `fvm_channel_value_send` | function | [include/flashvm/channel_value.h:41](../../../include/flashvm/channel_value.h#L41) |
| `fvm_config_init` | function | [include/flashvm/flashvm.h:70](../../../include/flashvm/flashvm.h#L70) |
| `fvm_value_number` | function | [include/flashvm/value.h:50](../../../include/flashvm/value.h#L50) |
| `fvm_value_undefined` | function | [include/flashvm/value.h:46](../../../include/flashvm/value.h#L46) |
| `fvm_worker_destroy` | function | [include/flashvm/worker.h:98](../../../include/flashvm/worker.h#L98) |
| `fvm_worker_join` | function | [include/flashvm/worker.h:60](../../../include/flashvm/worker.h#L60) |
| `fvm_worker_run` | function | [include/flashvm/worker.h:25](../../../include/flashvm/worker.h#L25) |
<!-- source-links:end -->
