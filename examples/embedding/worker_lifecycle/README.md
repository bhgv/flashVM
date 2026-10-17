# Suspended worker / Отложенный запуск

C89 example (`main.c`) creates a suspended OS thread, verifies its state,
starts it, joins and checks its result (42). The worker executes in a separate
FlashVM context. Build as part of the project and run `flashvm_examples_embedding_worker_lifecycle_main`.

Пример на C89 создаёт ожидающий поток ОС, проверяет состояние, запускает,
ожидает завершения и проверяет результат 42. Контекст FlashVM отдельный.

Этот main демонстрирует C worker API. Script-расширения thread описаны отдельно
в [ACTIONSCRIPT_EXTENSIONS.md](../../../doc/ACTIONSCRIPT_EXTENSIONS.md).

<!-- example-catalog-api:start -->
## Назначение и особенности

Создаёт worker в приостановленном состоянии, читает его state, запускает и получает 42 через join. Позволяет отделить создание потока от разрешения начать работу.

**Текущая CMake-цель:** `flashvm_examples_embedding_worker_lifecycle_main`.


## Демонстрируемые функции C API

Имена ведут к публичным декларациям. Во второй колонке указан файл руководства
и раздел с контрактом; общие функции setup/cleanup включены для воспроизводимости.

| Функция | Где читать подробнее |
| --- | --- |
| [`fvm_config_init`](../../../include/flashvm/flashvm.h) | [API_INDEX.md#runtime-and-context](../../../doc/API_INDEX.md#runtime-and-context) — Жизненный цикл runtime/context; точные сигнатуры и ownership — в заголовке. |
| [`fvm_value_number`](../../../include/flashvm/value.h) | [API_INDEX.md#values-and-borrowed-views](../../../doc/API_INDEX.md#values-and-borrowed-views) — Типы значений, borrowed-данные и преобразования. |
| [`fvm_worker_destroy`](../../../include/flashvm/worker.h) | [WORKERS.md](../../../doc/WORKERS.md) — Изолированные worker-контексты и join. |
| [`fvm_worker_join`](../../../include/flashvm/worker.h) | [WORKERS.md](../../../doc/WORKERS.md) — Изолированные worker-контексты и join. |
| [`fvm_worker_run_suspended`](../../../include/flashvm/worker.h) | [WORKER_LIFECYCLE.md](../../../doc/WORKER_LIFECYCLE.md) — Создание worker без немедленного запуска. |
| [`fvm_worker_start`](../../../include/flashvm/worker.h) | [WORKER_LIFECYCLE.md](../../../doc/WORKER_LIFECYCLE.md) — Разрешение запуска приостановленного worker. |
| [`fvm_worker_state`](../../../include/flashvm/worker.h) | [WORKER_LIFECYCLE.md](../../../doc/WORKER_LIFECYCLE.md) — Запрос состояния worker. |

[Описание в общем каталоге](../../README.md#example-embedding-worker_lifecycle).
<!-- example-catalog-api:end -->
