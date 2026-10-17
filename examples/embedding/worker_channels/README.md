# Worker-to-host channel exchange (C89)

`main.c` demonstrates a native worker running in a separate FlashVM context. The host sends a copied UTF-8 string through one FIFO channel, the worker returns its byte length through a second channel, and `join` retrieves the worker's final value. The two channels provide full-duplex communication without sharing Flash objects.

Build with `-DFVM_BUILD_EXAMPLES=ON` and run `flashvm_examples_embedding_worker_channels_main`. Channel handles are passed through native callback userdata, **not** copied into an AVM1 value. Close channels to wake a blocked receiver; always join workers before destroying their channels.

This example does not implement `thread.run`, `<...>` or `<-` in ActionScript.

<!-- example-catalog-api:start -->
## Назначение и особенности

Создаёт два FIFO для обмена host↔worker и передаёт строку с ответом о её длине. Показывает неблокирующий probe, закрытие каналов и обязательный join до их уничтожения.

**Текущая CMake-цель:** `flashvm_examples_embedding_worker_channels_main`.


## Демонстрируемые функции C API

Имена ведут к публичным декларациям. Во второй колонке указан файл руководства
и раздел с контрактом; общие функции setup/cleanup включены для воспроизводимости.

| Функция | Где читать подробнее |
| --- | --- |
| [`fvm_channel_available`](../../../include/flashvm/channel.h) | [CHANNELS.md#implemented-api](../../../doc/CHANNELS.md#implemented-api) — FIFO, передача значений и закрытие. |
| [`fvm_channel_close`](../../../include/flashvm/channel.h) | [CHANNELS.md#implemented-api](../../../doc/CHANNELS.md#implemented-api) — FIFO, передача значений и закрытие. |
| [`fvm_channel_create`](../../../include/flashvm/channel.h) | [CHANNELS.md#implemented-api](../../../doc/CHANNELS.md#implemented-api) — FIFO, передача значений и закрытие. |
| [`fvm_channel_destroy`](../../../include/flashvm/channel.h) | [CHANNELS.md#implemented-api](../../../doc/CHANNELS.md#implemented-api) — FIFO, передача значений и закрытие. |
| [`fvm_channel_receive`](../../../include/flashvm/channel.h) | [CHANNELS.md#implemented-api](../../../doc/CHANNELS.md#implemented-api) — FIFO, передача значений и закрытие. |
| [`fvm_channel_send`](../../../include/flashvm/channel.h) | [CHANNELS.md#implemented-api](../../../doc/CHANNELS.md#implemented-api) — FIFO, передача значений и закрытие. |
| [`fvm_channel_try_receive`](../../../include/flashvm/channel.h) | [CHANNELS.md#implemented-api](../../../doc/CHANNELS.md#implemented-api) — FIFO, передача значений и закрытие. |
| [`fvm_config_init`](../../../include/flashvm/flashvm.h) | [API_INDEX.md#runtime-and-context](../../../doc/API_INDEX.md#runtime-and-context) — Жизненный цикл runtime/context; точные сигнатуры и ownership — в заголовке. |
| [`fvm_value_number`](../../../include/flashvm/value.h) | [API_INDEX.md#values-and-borrowed-views](../../../doc/API_INDEX.md#values-and-borrowed-views) — Типы значений, borrowed-данные и преобразования. |
| [`fvm_value_string`](../../../include/flashvm/value.h) | [API_INDEX.md#values-and-borrowed-views](../../../doc/API_INDEX.md#values-and-borrowed-views) — Типы значений, borrowed-данные и преобразования. |
| [`fvm_value_undefined`](../../../include/flashvm/value.h) | [API_INDEX.md#values-and-borrowed-views](../../../doc/API_INDEX.md#values-and-borrowed-views) — Типы значений, borrowed-данные и преобразования. |
| [`fvm_worker_destroy`](../../../include/flashvm/worker.h) | [WORKERS.md](../../../doc/WORKERS.md) — Изолированные worker-контексты и join. |
| [`fvm_worker_join`](../../../include/flashvm/worker.h) | [WORKERS.md](../../../doc/WORKERS.md) — Изолированные worker-контексты и join. |
| [`fvm_worker_run_suspended`](../../../include/flashvm/worker.h) | [WORKER_LIFECYCLE.md](../../../doc/WORKER_LIFECYCLE.md) — Создание worker без немедленного запуска. |
| [`fvm_worker_start`](../../../include/flashvm/worker.h) | [WORKER_LIFECYCLE.md](../../../doc/WORKER_LIFECYCLE.md) — Разрешение запуска приостановленного worker. |

[Описание в общем каталоге](../../README.md#example-embedding-worker_channels).
<!-- example-catalog-api:end -->
