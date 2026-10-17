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

Имена ведут к публичным декларациям. Во второй колонке указан файл руководства
и раздел с контрактом; общие функции setup/cleanup включены для воспроизводимости.

| Функция | Где читать подробнее |
| --- | --- |
| [`fvm_channel_value_copy`](../../../include/flashvm/channel_value.h) | [CHANNEL_VALUES.md#api](../../../doc/CHANNEL_VALUES.md#api) — Владеющие channel handles и явное копирование. |
| [`fvm_channel_value_create`](../../../include/flashvm/channel_value.h) | [CHANNEL_VALUES.md#api](../../../doc/CHANNEL_VALUES.md#api) — Владеющие channel handles и явное копирование. |
| [`fvm_channel_value_receive`](../../../include/flashvm/channel_value.h) | [CHANNEL_VALUES.md#api](../../../doc/CHANNEL_VALUES.md#api) — Владеющие channel handles и явное копирование. |
| [`fvm_channel_value_release`](../../../include/flashvm/channel_value.h) | [CHANNEL_VALUES.md#api](../../../doc/CHANNEL_VALUES.md#api) — Владеющие channel handles и явное копирование. |
| [`fvm_channel_value_send`](../../../include/flashvm/channel_value.h) | [CHANNEL_VALUES.md#api](../../../doc/CHANNEL_VALUES.md#api) — Владеющие channel handles и явное копирование. |
| [`fvm_config_init`](../../../include/flashvm/flashvm.h) | [API_INDEX.md#runtime-and-context](../../../doc/API_INDEX.md#runtime-and-context) — Жизненный цикл runtime/context; точные сигнатуры и ownership — в заголовке. |
| [`fvm_value_number`](../../../include/flashvm/value.h) | [API_INDEX.md#values-and-borrowed-views](../../../doc/API_INDEX.md#values-and-borrowed-views) — Типы значений, borrowed-данные и преобразования. |
| [`fvm_value_undefined`](../../../include/flashvm/value.h) | [API_INDEX.md#values-and-borrowed-views](../../../doc/API_INDEX.md#values-and-borrowed-views) — Типы значений, borrowed-данные и преобразования. |
| [`fvm_worker_destroy`](../../../include/flashvm/worker.h) | [WORKERS.md](../../../doc/WORKERS.md) — Изолированные worker-контексты и join. |
| [`fvm_worker_join`](../../../include/flashvm/worker.h) | [WORKERS.md](../../../doc/WORKERS.md) — Изолированные worker-контексты и join. |
| [`fvm_worker_run`](../../../include/flashvm/worker.h) | [WORKERS.md](../../../doc/WORKERS.md) — Изолированные worker-контексты и join. |

[Описание в общем каталоге](../../README.md#example-extending-channel_worker).
<!-- example-catalog-api:end -->
