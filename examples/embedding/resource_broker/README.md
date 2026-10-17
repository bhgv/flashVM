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
| [`fvm_broker_release`](../../../include/flashvm/capability.h) | [RESOURCE_TRANSPORT.md#русский](../../../doc/RESOURCE_TRANSPORT.md#русский) — Брокер, экспорт и импорт общих ресурсов. |
| [`fvm_broker_start`](../../../include/flashvm/capability.h) | [RESOURCE_TRANSPORT.md#русский](../../../doc/RESOURCE_TRANSPORT.md#русский) — Брокер, экспорт и импорт общих ресурсов. |
| [`fvm_broker_stop`](../../../include/flashvm/capability.h) | [RESOURCE_TRANSPORT.md#русский](../../../doc/RESOURCE_TRANSPORT.md#русский) — Брокер, экспорт и импорт общих ресурсов. |
| [`fvm_channel_open_file`](../../../include/flashvm/channel.h) | [FILE_CHANNELS.md](../../../doc/FILE_CHANNELS.md) — Открытие файлового канала и режимы доступа. |
| [`fvm_channel_receive_value`](../../../include/flashvm/transfer.h) | [RESOURCE_TRANSPORT.md#русский](../../../doc/RESOURCE_TRANSPORT.md#русский) — Сериализованные графы и ресурсные ссылки. |
| [`fvm_channel_release`](../../../include/flashvm/channel.h) | [CHANNELS.md#implemented-api](../../../doc/CHANNELS.md#implemented-api) — FIFO, передача значений и закрытие. |
| [`fvm_channel_send_value`](../../../include/flashvm/transfer.h) | [RESOURCE_TRANSPORT.md#русский](../../../doc/RESOURCE_TRANSPORT.md#русский) — Сериализованные графы и ресурсные ссылки. |
| [`fvm_config_init`](../../../include/flashvm/flashvm.h) | [API_INDEX.md#runtime-and-context](../../../doc/API_INDEX.md#runtime-and-context) — Жизненный цикл runtime/context; точные сигнатуры и ownership — в заголовке. |
| [`fvm_context_create`](../../../include/flashvm/flashvm.h) | [API_INDEX.md#runtime-and-context](../../../doc/API_INDEX.md#runtime-and-context) — Жизненный цикл runtime/context; точные сигнатуры и ownership — в заголовке. |
| [`fvm_context_destroy`](../../../include/flashvm/flashvm.h) | [API_INDEX.md#runtime-and-context](../../../doc/API_INDEX.md#runtime-and-context) — Жизненный цикл runtime/context; точные сигнатуры и ownership — в заголовке. |
| [`fvm_context_set_broker`](../../../include/flashvm/capability.h) | [RESOURCE_TRANSPORT.md#русский](../../../doc/RESOURCE_TRANSPORT.md#русский) — Брокер, экспорт и импорт общих ресурсов. |
| [`fvm_extensions_wrap_mutex`](../../../include/flashvm/mutex.h) | [MUTEX_CHECKPOINT.md](../../../doc/MUTEX_CHECKPOINT.md) — Идентичность mutex, захват и освобождение. |
| [`fvm_mutex_create`](../../../include/flashvm/mutex.h) | [MUTEX_CHECKPOINT.md](../../../doc/MUTEX_CHECKPOINT.md) — Идентичность mutex, захват и освобождение. |
| [`fvm_mutex_release`](../../../include/flashvm/mutex.h) | [MUTEX_CHECKPOINT.md](../../../doc/MUTEX_CHECKPOINT.md) — Идентичность mutex, захват и освобождение. |
| [`fvm_mutex_touch`](../../../include/flashvm/mutex.h) | [MUTEX_CHECKPOINT.md](../../../doc/MUTEX_CHECKPOINT.md) — Идентичность mutex, захват и освобождение. |
| [`fvm_object_call`](../../../include/flashvm/value.h) | [AVM1_FUNCTIONS.md](../../../doc/AVM1_FUNCTIONS.md) — Метод, receiver и аргументы. |
| [`fvm_runtime_create`](../../../include/flashvm/flashvm.h) | [API_INDEX.md#runtime-and-context](../../../doc/API_INDEX.md#runtime-and-context) — Жизненный цикл runtime/context; точные сигнатуры и ownership — в заголовке. |
| [`fvm_runtime_destroy`](../../../include/flashvm/flashvm.h) | [API_INDEX.md#runtime-and-context](../../../doc/API_INDEX.md#runtime-and-context) — Жизненный цикл runtime/context; точные сигнатуры и ownership — в заголовке. |
| [`fvm_value_undefined`](../../../include/flashvm/value.h) | [API_INDEX.md#values-and-borrowed-views](../../../doc/API_INDEX.md#values-and-borrowed-views) — Типы значений, borrowed-данные и преобразования. |

[Описание в общем каталоге](../../README.md#example-embedding-resource_broker).
<!-- example-catalog-api:end -->
