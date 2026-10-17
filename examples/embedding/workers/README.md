# Native worker / Нативный рабочий поток

EN: `main.c` demonstrates starting a C callback in a separate OS thread with a fresh FlashVM runtime and context. The input string is copied before thread creation. `join` waits for completion; the returned value is borrowed from the worker handle until `destroy`. Build via CMake target `flashvm_examples_embedding_workers_main` (test) or compile `main.c` against FlashVM.

RU: `main.c` показывает запуск C-функции в отдельном системном потоке с новым Runtime и Context. Входная строка копируется перед запуском. `join` ожидает завершения, результат принадлежит дескриптору потока до вызова `destroy`. Собирайте цель `flashvm_examples_embedding_workers_main` или `main.c` с библиотекой FlashVM.

Этот main использует native worker callback. Script thread.run описан отдельно в [ACTIONSCRIPT_EXTENSIONS.md](../../../doc/ACTIONSCRIPT_EXTENSIONS.md).

<!-- example-catalog-api:start -->
## Назначение и особенности

Запускает native callback в отдельном OS-потоке с новым runtime/context и копией входного значения. После join читает результат до destroy; userdata и управляемые объекты не становятся автоматически общими.

**Текущая CMake-цель:** `flashvm_examples_embedding_workers_main`.


## Демонстрируемые функции C API

Имена ведут к публичным декларациям. Во второй колонке указан файл руководства
и раздел с контрактом; общие функции setup/cleanup включены для воспроизводимости.

| Функция | Где читать подробнее |
| --- | --- |
| [`fvm_config_init`](../../../include/flashvm/flashvm.h) | [API_INDEX.md#runtime-and-context](../../../doc/API_INDEX.md#runtime-and-context) — Жизненный цикл runtime/context; точные сигнатуры и ownership — в заголовке. |
| [`fvm_get_global`](../../../include/flashvm/value.h) | [API_INDEX.md#values-and-borrowed-views](../../../doc/API_INDEX.md#values-and-borrowed-views) — Типы значений, borrowed-данные и преобразования. |
| [`fvm_set_global`](../../../include/flashvm/value.h) | [API_INDEX.md#values-and-borrowed-views](../../../doc/API_INDEX.md#values-and-borrowed-views) — Типы значений, borrowed-данные и преобразования. |
| [`fvm_value_number`](../../../include/flashvm/value.h) | [API_INDEX.md#values-and-borrowed-views](../../../doc/API_INDEX.md#values-and-borrowed-views) — Типы значений, borrowed-данные и преобразования. |
| [`fvm_value_string`](../../../include/flashvm/value.h) | [API_INDEX.md#values-and-borrowed-views](../../../doc/API_INDEX.md#values-and-borrowed-views) — Типы значений, borrowed-данные и преобразования. |
| [`fvm_worker_destroy`](../../../include/flashvm/worker.h) | [WORKERS.md](../../../doc/WORKERS.md) — Изолированные worker-контексты и join. |
| [`fvm_worker_join`](../../../include/flashvm/worker.h) | [WORKERS.md](../../../doc/WORKERS.md) — Изолированные worker-контексты и join. |
| [`fvm_worker_run`](../../../include/flashvm/worker.h) | [WORKERS.md](../../../doc/WORKERS.md) — Изолированные worker-контексты и join. |

[Описание в общем каталоге](../../README.md#example-embedding-workers).
<!-- example-catalog-api:end -->
