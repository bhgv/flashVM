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
| [`fvm_broker_release`](../../../include/flashvm/capability.h#L40) | [RESOURCE_TRANSPORT.md#русский](../../../doc/RESOURCE_TRANSPORT.md#русский) — Брокер, экспорт и импорт общих ресурсов. |
| [`fvm_broker_start`](../../../include/flashvm/capability.h#L19) | [RESOURCE_TRANSPORT.md#русский](../../../doc/RESOURCE_TRANSPORT.md#русский) — Брокер, экспорт и импорт общих ресурсов. |
| [`fvm_broker_stop`](../../../include/flashvm/capability.h#L38) | [RESOURCE_TRANSPORT.md#русский](../../../doc/RESOURCE_TRANSPORT.md#русский) — Брокер, экспорт и импорт общих ресурсов. |
| [`fvm_channel_open_file`](../../../include/flashvm/channel.h#L28) | [FILE_CHANNELS.md](../../../doc/FILE_CHANNELS.md) — Открытие файлового канала и режимы доступа. |
| [`fvm_channel_receive_value`](../../../include/flashvm/transfer.h#L25) | [RESOURCE_TRANSPORT.md#русский](../../../doc/RESOURCE_TRANSPORT.md#русский) — Сериализованные графы и ресурсные ссылки. |
| [`fvm_channel_release`](../../../include/flashvm/channel.h#L47) | [CHANNELS.md#implemented-api](../../../doc/CHANNELS.md#implemented-api) — FIFO, передача значений и закрытие. |
| [`fvm_channel_send_value`](../../../include/flashvm/transfer.h#L22) | [RESOURCE_TRANSPORT.md#русский](../../../doc/RESOURCE_TRANSPORT.md#русский) — Сериализованные графы и ресурсные ссылки. |
| [`fvm_config_init`](../../../include/flashvm/flashvm.h#L62) | [API_INDEX.md#runtime-and-context](../../../doc/API_INDEX.md#runtime-and-context) — Жизненный цикл runtime/context; точные сигнатуры и ownership — в заголовке. |
| [`fvm_context_create`](../../../include/flashvm/flashvm.h#L68) | [API_INDEX.md#runtime-and-context](../../../doc/API_INDEX.md#runtime-and-context) — Жизненный цикл runtime/context; точные сигнатуры и ownership — в заголовке. |
| [`fvm_context_destroy`](../../../include/flashvm/flashvm.h#L70) | [API_INDEX.md#runtime-and-context](../../../doc/API_INDEX.md#runtime-and-context) — Жизненный цикл runtime/context; точные сигнатуры и ownership — в заголовке. |
| [`fvm_context_set_broker`](../../../include/flashvm/capability.h#L42) | [RESOURCE_TRANSPORT.md#русский](../../../doc/RESOURCE_TRANSPORT.md#русский) — Брокер, экспорт и импорт общих ресурсов. |
| [`fvm_extensions_wrap_mutex`](../../../include/flashvm/mutex.h#L40) | [MUTEX_CHECKPOINT.md](../../../doc/MUTEX_CHECKPOINT.md) — Идентичность mutex, захват и освобождение. |
| [`fvm_mutex_create`](../../../include/flashvm/mutex.h#L18) | [MUTEX_CHECKPOINT.md](../../../doc/MUTEX_CHECKPOINT.md) — Идентичность mutex, захват и освобождение. |
| [`fvm_mutex_release`](../../../include/flashvm/mutex.h#L22) | [MUTEX_CHECKPOINT.md](../../../doc/MUTEX_CHECKPOINT.md) — Идентичность mutex, захват и освобождение. |
| [`fvm_mutex_touch`](../../../include/flashvm/mutex.h#L34) | [MUTEX_CHECKPOINT.md](../../../doc/MUTEX_CHECKPOINT.md) — Идентичность mutex, захват и освобождение. |
| [`fvm_object_call`](../../../include/flashvm/value.h#L126) | [AVM1_FUNCTIONS.md](../../../doc/AVM1_FUNCTIONS.md) — Метод, receiver и аргументы. |
| [`fvm_runtime_create`](../../../include/flashvm/flashvm.h#L64) | [API_INDEX.md#runtime-and-context](../../../doc/API_INDEX.md#runtime-and-context) — Жизненный цикл runtime/context; точные сигнатуры и ownership — в заголовке. |
| [`fvm_runtime_destroy`](../../../include/flashvm/flashvm.h#L66) | [API_INDEX.md#runtime-and-context](../../../doc/API_INDEX.md#runtime-and-context) — Жизненный цикл runtime/context; точные сигнатуры и ownership — в заголовке. |
| [`fvm_value_undefined`](../../../include/flashvm/value.h#L40) | [API_INDEX.md#values-and-borrowed-views](../../../doc/API_INDEX.md#values-and-borrowed-views) — Типы значений, borrowed-данные и преобразования. |

[Описание в общем каталоге](../../README.md#example-embedding-resource_broker).
<!-- example-catalog-api:end -->

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_broker_release` | function | [include/flashvm/capability.h:40](../../../include/flashvm/capability.h#L40) |
| `fvm_broker_start` | function | [include/flashvm/capability.h:19](../../../include/flashvm/capability.h#L19) |
| `fvm_broker_stop` | function | [include/flashvm/capability.h:38](../../../include/flashvm/capability.h#L38) |
| `fvm_channel_open_file` | function | [include/flashvm/channel.h:28](../../../include/flashvm/channel.h#L28) |
| `fvm_channel_receive_value` | function | [include/flashvm/transfer.h:25](../../../include/flashvm/transfer.h#L25) |
| `fvm_channel_release` | function | [include/flashvm/channel.h:47](../../../include/flashvm/channel.h#L47) |
| `fvm_channel_send_value` | function | [include/flashvm/transfer.h:22](../../../include/flashvm/transfer.h#L22) |
| `fvm_config_init` | function | [include/flashvm/flashvm.h:62](../../../include/flashvm/flashvm.h#L62) |
| `fvm_context_create` | function | [include/flashvm/flashvm.h:68](../../../include/flashvm/flashvm.h#L68) |
| `fvm_context_destroy` | function | [include/flashvm/flashvm.h:70](../../../include/flashvm/flashvm.h#L70) |
| `fvm_context_set_broker` | function | [include/flashvm/capability.h:42](../../../include/flashvm/capability.h#L42) |
| `fvm_extensions_wrap_mutex` | function | [include/flashvm/mutex.h:40](../../../include/flashvm/mutex.h#L40) |
| `fvm_mutex_create` | function | [include/flashvm/mutex.h:18](../../../include/flashvm/mutex.h#L18) |
| `fvm_mutex_release` | function | [include/flashvm/mutex.h:22](../../../include/flashvm/mutex.h#L22) |
| `fvm_mutex_touch` | function | [include/flashvm/mutex.h:34](../../../include/flashvm/mutex.h#L34) |
| `fvm_object_call` | function | [include/flashvm/value.h:126](../../../include/flashvm/value.h#L126) |
| `fvm_runtime_create` | function | [include/flashvm/flashvm.h:64](../../../include/flashvm/flashvm.h#L64) |
| `fvm_runtime_destroy` | function | [include/flashvm/flashvm.h:66](../../../include/flashvm/flashvm.h#L66) |
| `fvm_value_undefined` | function | [include/flashvm/value.h:40](../../../include/flashvm/value.h#L40) |
<!-- source-links:end -->
