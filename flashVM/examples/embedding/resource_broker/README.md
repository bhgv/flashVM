# embedding/resource_broker

Экспортирует живой mutex через TCP-брокер, сохраняет ссылку в файловом канале и импортирует её в другом контексте. Восстанавливается доступ к исходному ресурсу, а не снимок его состояния; требуется OS_PORTS.

## Реализация и запуск

Пример C/C++ собирается с `FVM_BUILD_EXAMPLES=ON`. Из корня репозитория:

```sh
cmake -S . -B build-examples -DFVM_DEFS_FILE=cmake/profiles/minimal.defs -DFVM_BUILD_EXAMPLES=ON
cmake --build build-examples --target flashvm_examples_embedding_resource_broker_main --parallel
./build-examples/flashvm_examples_embedding_resource_broker_main
```

Операции зависят от выбранного DEFS-профиля; OS_PORTS, JIT, графика и codecs
не включаются наличием этого README. Исходники ниже показывают порядок API-вызовов
и освобождения ресурсов.

<!-- example-catalog-api:start -->
**Текущая CMake-цель:** `flashvm_examples_embedding_resource_broker_main`.


## Демонстрируемые функции C API

Имена ведут к публичным декларациям. Во второй колонке указан файл руководства
и раздел с контрактом; общие функции setup/cleanup включены для воспроизводимости.

| Функция | Где читать подробнее |
| --- | --- |
| [`fvm_broker_release`](../../../include/flashvm/capability.h#L54) | [RESOURCE_TRANSPORT.md#русский](../../../doc/RESOURCE_TRANSPORT.md#русский) — Брокер, экспорт и импорт общих ресурсов. |
| [`fvm_broker_start`](../../../include/flashvm/capability.h#L21) | [RESOURCE_TRANSPORT.md#русский](../../../doc/RESOURCE_TRANSPORT.md#русский) — Брокер, экспорт и импорт общих ресурсов. |
| [`fvm_broker_stop`](../../../include/flashvm/capability.h#L50) | [RESOURCE_TRANSPORT.md#русский](../../../doc/RESOURCE_TRANSPORT.md#русский) — Брокер, экспорт и импорт общих ресурсов. |
| [`fvm_channel_open_file`](../../../include/flashvm/channel.h#L63) | [FILE_CHANNELS.md](../../../doc/FILE_CHANNELS.md) — Открытие файлового канала и режимы доступа. |
| [`fvm_channel_receive_value`](../../../include/flashvm/transfer.h#L29) | [RESOURCE_TRANSPORT.md#русский](../../../doc/RESOURCE_TRANSPORT.md#русский) — Сериализованные графы и ресурсные ссылки. |
| [`fvm_channel_release`](../../../include/flashvm/channel.h#L96) | [CHANNELS.md#implemented-api](../../../doc/CHANNELS.md#implemented-api) — FIFO, передача значений и закрытие. |
| [`fvm_channel_send_value`](../../../include/flashvm/transfer.h#L24) | [RESOURCE_TRANSPORT.md#русский](../../../doc/RESOURCE_TRANSPORT.md#русский) — Сериализованные графы и ресурсные ссылки. |
| [`fvm_config_init`](../../../include/flashvm/flashvm.h#L70) | [API_INDEX.md#runtime-and-context](../../../doc/API_INDEX.md#runtime-and-context) — Жизненный цикл runtime/context; точные сигнатуры и ownership — в заголовке. |
| [`fvm_context_create`](../../../include/flashvm/flashvm.h#L82) | [API_INDEX.md#runtime-and-context](../../../doc/API_INDEX.md#runtime-and-context) — Жизненный цикл runtime/context; точные сигнатуры и ownership — в заголовке. |
| [`fvm_context_destroy`](../../../include/flashvm/flashvm.h#L86) | [API_INDEX.md#runtime-and-context](../../../doc/API_INDEX.md#runtime-and-context) — Жизненный цикл runtime/context; точные сигнатуры и ownership — в заголовке. |
| [`fvm_context_set_broker`](../../../include/flashvm/capability.h#L58) | [RESOURCE_TRANSPORT.md#русский](../../../doc/RESOURCE_TRANSPORT.md#русский) — Брокер, экспорт и импорт общих ресурсов. |
| [`fvm_extensions_wrap_mutex`](../../../include/flashvm/mutex.h#L64) | [MUTEX_CHECKPOINT.md](../../../doc/MUTEX_CHECKPOINT.md) — Идентичность mutex, захват и освобождение. |
| [`fvm_mutex_create`](../../../include/flashvm/mutex.h#L22) | [MUTEX_CHECKPOINT.md](../../../doc/MUTEX_CHECKPOINT.md) — Идентичность mutex, захват и освобождение. |
| [`fvm_mutex_release`](../../../include/flashvm/mutex.h#L30) | [MUTEX_CHECKPOINT.md](../../../doc/MUTEX_CHECKPOINT.md) — Идентичность mutex, захват и освобождение. |
| [`fvm_mutex_touch`](../../../include/flashvm/mutex.h#L52) | [MUTEX_CHECKPOINT.md](../../../doc/MUTEX_CHECKPOINT.md) — Идентичность mutex, захват и освобождение. |
| [`fvm_object_call`](../../../include/flashvm/value.h#L184) | [AVM1_FUNCTIONS.md](../../../doc/AVM1_FUNCTIONS.md) — Метод, receiver и аргументы. |
| [`fvm_runtime_create`](../../../include/flashvm/flashvm.h#L74) | [API_INDEX.md#runtime-and-context](../../../doc/API_INDEX.md#runtime-and-context) — Жизненный цикл runtime/context; точные сигнатуры и ownership — в заголовке. |
| [`fvm_runtime_destroy`](../../../include/flashvm/flashvm.h#L78) | [API_INDEX.md#runtime-and-context](../../../doc/API_INDEX.md#runtime-and-context) — Жизненный цикл runtime/context; точные сигнатуры и ownership — в заголовке. |
| [`fvm_value_undefined`](../../../include/flashvm/value.h#L46) | [API_INDEX.md#values-and-borrowed-views](../../../doc/API_INDEX.md#values-and-borrowed-views) — Типы значений, borrowed-данные и преобразования. |

[Описание в общем каталоге](../../README.md#example-embedding-resource_broker).
<!-- example-catalog-api:end -->

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_broker_release` | function | [include/flashvm/capability.h:54](../../../include/flashvm/capability.h#L54) |
| `fvm_broker_start` | function | [include/flashvm/capability.h:21](../../../include/flashvm/capability.h#L21) |
| `fvm_broker_stop` | function | [include/flashvm/capability.h:50](../../../include/flashvm/capability.h#L50) |
| `fvm_channel_open_file` | function | [include/flashvm/channel.h:63](../../../include/flashvm/channel.h#L63) |
| `fvm_channel_receive_value` | function | [include/flashvm/transfer.h:29](../../../include/flashvm/transfer.h#L29) |
| `fvm_channel_release` | function | [include/flashvm/channel.h:96](../../../include/flashvm/channel.h#L96) |
| `fvm_channel_send_value` | function | [include/flashvm/transfer.h:24](../../../include/flashvm/transfer.h#L24) |
| `fvm_config_init` | function | [include/flashvm/flashvm.h:70](../../../include/flashvm/flashvm.h#L70) |
| `fvm_context_create` | function | [include/flashvm/flashvm.h:82](../../../include/flashvm/flashvm.h#L82) |
| `fvm_context_destroy` | function | [include/flashvm/flashvm.h:86](../../../include/flashvm/flashvm.h#L86) |
| `fvm_context_set_broker` | function | [include/flashvm/capability.h:58](../../../include/flashvm/capability.h#L58) |
| `fvm_extensions_wrap_mutex` | function | [include/flashvm/mutex.h:64](../../../include/flashvm/mutex.h#L64) |
| `fvm_mutex_create` | function | [include/flashvm/mutex.h:22](../../../include/flashvm/mutex.h#L22) |
| `fvm_mutex_release` | function | [include/flashvm/mutex.h:30](../../../include/flashvm/mutex.h#L30) |
| `fvm_mutex_touch` | function | [include/flashvm/mutex.h:52](../../../include/flashvm/mutex.h#L52) |
| `fvm_object_call` | function | [include/flashvm/value.h:184](../../../include/flashvm/value.h#L184) |
| `fvm_runtime_create` | function | [include/flashvm/flashvm.h:74](../../../include/flashvm/flashvm.h#L74) |
| `fvm_runtime_destroy` | function | [include/flashvm/flashvm.h:78](../../../include/flashvm/flashvm.h#L78) |
| `fvm_value_undefined` | function | [include/flashvm/value.h:46](../../../include/flashvm/value.h#L46) |
<!-- source-links:end -->
