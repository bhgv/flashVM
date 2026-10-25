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
| [`fvm_config_init`](../../../include/flashvm/flashvm.h#L70) | [API_INDEX.md#runtime-and-context](../../../doc/API_INDEX.md#runtime-and-context) — Жизненный цикл runtime/context; точные сигнатуры и ownership — в заголовке. |
| [`fvm_value_number`](../../../include/flashvm/value.h#L50) | [API_INDEX.md#values-and-borrowed-views](../../../doc/API_INDEX.md#values-and-borrowed-views) — Типы значений, borrowed-данные и преобразования. |
| [`fvm_worker_destroy`](../../../include/flashvm/worker.h#L98) | [WORKERS.md](../../../doc/WORKERS.md) — Изолированные worker-контексты и join. |
| [`fvm_worker_join`](../../../include/flashvm/worker.h#L60) | [WORKERS.md](../../../doc/WORKERS.md) — Изолированные worker-контексты и join. |
| [`fvm_worker_run_suspended`](../../../include/flashvm/worker.h#L32) | [WORKER_LIFECYCLE.md](../../../doc/WORKER_LIFECYCLE.md) — Создание worker без немедленного запуска. |
| [`fvm_worker_start`](../../../include/flashvm/worker.h#L38) | [WORKER_LIFECYCLE.md](../../../doc/WORKER_LIFECYCLE.md) — Разрешение запуска приостановленного worker. |
| [`fvm_worker_state`](../../../include/flashvm/worker.h#L54) | [WORKER_LIFECYCLE.md](../../../doc/WORKER_LIFECYCLE.md) — Запрос состояния worker. |

[Описание в общем каталоге](../../README.md#example-embedding-worker_lifecycle).
<!-- example-catalog-api:end -->

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_config_init` | function | [include/flashvm/flashvm.h:70](../../../include/flashvm/flashvm.h#L70) |
| `fvm_value_number` | function | [include/flashvm/value.h:50](../../../include/flashvm/value.h#L50) |
| `fvm_worker_destroy` | function | [include/flashvm/worker.h:98](../../../include/flashvm/worker.h#L98) |
| `fvm_worker_join` | function | [include/flashvm/worker.h:60](../../../include/flashvm/worker.h#L60) |
| `fvm_worker_run_suspended` | function | [include/flashvm/worker.h:32](../../../include/flashvm/worker.h#L32) |
| `fvm_worker_start` | function | [include/flashvm/worker.h:38](../../../include/flashvm/worker.h#L38) |
| `fvm_worker_state` | function | [include/flashvm/worker.h:54](../../../include/flashvm/worker.h#L54) |
<!-- source-links:end -->
