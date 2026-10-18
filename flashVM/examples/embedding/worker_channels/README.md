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
| [`fvm_channel_available`](../../../include/flashvm/channel.h#L74) | [CHANNELS.md#implemented-api](../../../doc/CHANNELS.md#implemented-api) — FIFO, передача значений и закрытие. |
| [`fvm_channel_close`](../../../include/flashvm/channel.h#L40) | [CHANNELS.md#implemented-api](../../../doc/CHANNELS.md#implemented-api) — FIFO, передача значений и закрытие. |
| [`fvm_channel_create`](../../../include/flashvm/channel.h#L23) | [CHANNELS.md#implemented-api](../../../doc/CHANNELS.md#implemented-api) — FIFO, передача значений и закрытие. |
| [`fvm_channel_destroy`](../../../include/flashvm/channel.h#L49) | [CHANNELS.md#implemented-api](../../../doc/CHANNELS.md#implemented-api) — FIFO, передача значений и закрытие. |
| [`fvm_channel_receive`](../../../include/flashvm/channel.h#L55) | [CHANNELS.md#implemented-api](../../../doc/CHANNELS.md#implemented-api) — FIFO, передача значений и закрытие. |
| [`fvm_channel_send`](../../../include/flashvm/channel.h#L53) | [CHANNELS.md#implemented-api](../../../doc/CHANNELS.md#implemented-api) — FIFO, передача значений и закрытие. |
| [`fvm_channel_try_receive`](../../../include/flashvm/channel.h#L70) | [CHANNELS.md#implemented-api](../../../doc/CHANNELS.md#implemented-api) — FIFO, передача значений и закрытие. |
| [`fvm_config_init`](../../../include/flashvm/flashvm.h#L62) | [API_INDEX.md#runtime-and-context](../../../doc/API_INDEX.md#runtime-and-context) — Жизненный цикл runtime/context; точные сигнатуры и ownership — в заголовке. |
| [`fvm_value_number`](../../../include/flashvm/value.h#L42) | [API_INDEX.md#values-and-borrowed-views](../../../doc/API_INDEX.md#values-and-borrowed-views) — Типы значений, borrowed-данные и преобразования. |
| [`fvm_value_string`](../../../include/flashvm/value.h#L46) | [API_INDEX.md#values-and-borrowed-views](../../../doc/API_INDEX.md#values-and-borrowed-views) — Типы значений, borrowed-данные и преобразования. |
| [`fvm_value_undefined`](../../../include/flashvm/value.h#L40) | [API_INDEX.md#values-and-borrowed-views](../../../doc/API_INDEX.md#values-and-borrowed-views) — Типы значений, borrowed-данные и преобразования. |
| [`fvm_worker_destroy`](../../../include/flashvm/worker.h#L71) | [WORKERS.md](../../../doc/WORKERS.md) — Изолированные worker-контексты и join. |
| [`fvm_worker_join`](../../../include/flashvm/worker.h#L45) | [WORKERS.md](../../../doc/WORKERS.md) — Изолированные worker-контексты и join. |
| [`fvm_worker_run_suspended`](../../../include/flashvm/worker.h#L26) | [WORKER_LIFECYCLE.md](../../../doc/WORKER_LIFECYCLE.md) — Создание worker без немедленного запуска. |
| [`fvm_worker_start`](../../../include/flashvm/worker.h#L30) | [WORKER_LIFECYCLE.md](../../../doc/WORKER_LIFECYCLE.md) — Разрешение запуска приостановленного worker. |

[Описание в общем каталоге](../../README.md#example-embedding-worker_channels).
<!-- example-catalog-api:end -->

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_channel_available` | function | [include/flashvm/channel.h:74](../../../include/flashvm/channel.h#L74) |
| `fvm_channel_close` | function | [include/flashvm/channel.h:40](../../../include/flashvm/channel.h#L40) |
| `fvm_channel_create` | function | [include/flashvm/channel.h:23](../../../include/flashvm/channel.h#L23) |
| `fvm_channel_destroy` | function | [include/flashvm/channel.h:49](../../../include/flashvm/channel.h#L49) |
| `fvm_channel_receive` | function | [include/flashvm/channel.h:55](../../../include/flashvm/channel.h#L55) |
| `fvm_channel_send` | function | [include/flashvm/channel.h:53](../../../include/flashvm/channel.h#L53) |
| `fvm_channel_try_receive` | function | [include/flashvm/channel.h:70](../../../include/flashvm/channel.h#L70) |
| `fvm_config_init` | function | [include/flashvm/flashvm.h:62](../../../include/flashvm/flashvm.h#L62) |
| `fvm_value_number` | function | [include/flashvm/value.h:42](../../../include/flashvm/value.h#L42) |
| `fvm_value_string` | function | [include/flashvm/value.h:46](../../../include/flashvm/value.h#L46) |
| `fvm_value_undefined` | function | [include/flashvm/value.h:40](../../../include/flashvm/value.h#L40) |
| `fvm_worker_destroy` | function | [include/flashvm/worker.h:71](../../../include/flashvm/worker.h#L71) |
| `fvm_worker_join` | function | [include/flashvm/worker.h:45](../../../include/flashvm/worker.h#L45) |
| `fvm_worker_run_suspended` | function | [include/flashvm/worker.h:26](../../../include/flashvm/worker.h#L26) |
| `fvm_worker_start` | function | [include/flashvm/worker.h:30](../../../include/flashvm/worker.h#L30) |
<!-- source-links:end -->
