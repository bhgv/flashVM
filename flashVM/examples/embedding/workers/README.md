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
| [`fvm_config_init`](../../../include/flashvm/flashvm.h#L70) | [API_INDEX.md#runtime-and-context](../../../doc/API_INDEX.md#runtime-and-context) — Жизненный цикл runtime/context; точные сигнатуры и ownership — в заголовке. |
| [`fvm_get_global`](../../../include/flashvm/value.h#L125) | [API_INDEX.md#values-and-borrowed-views](../../../doc/API_INDEX.md#values-and-borrowed-views) — Типы значений, borrowed-данные и преобразования. |
| [`fvm_set_global`](../../../include/flashvm/value.h#L129) | [API_INDEX.md#values-and-borrowed-views](../../../doc/API_INDEX.md#values-and-borrowed-views) — Типы значений, borrowed-данные и преобразования. |
| [`fvm_value_number`](../../../include/flashvm/value.h#L50) | [API_INDEX.md#values-and-borrowed-views](../../../doc/API_INDEX.md#values-and-borrowed-views) — Типы значений, borrowed-данные и преобразования. |
| [`fvm_value_string`](../../../include/flashvm/value.h#L58) | [API_INDEX.md#values-and-borrowed-views](../../../doc/API_INDEX.md#values-and-borrowed-views) — Типы значений, borrowed-данные и преобразования. |
| [`fvm_worker_destroy`](../../../include/flashvm/worker.h#L98) | [WORKERS.md](../../../doc/WORKERS.md) — Изолированные worker-контексты и join. |
| [`fvm_worker_join`](../../../include/flashvm/worker.h#L60) | [WORKERS.md](../../../doc/WORKERS.md) — Изолированные worker-контексты и join. |
| [`fvm_worker_run`](../../../include/flashvm/worker.h#L25) | [WORKERS.md](../../../doc/WORKERS.md) — Изолированные worker-контексты и join. |

[Описание в общем каталоге](../../README.md#example-embedding-workers).
<!-- example-catalog-api:end -->

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_config_init` | function | [include/flashvm/flashvm.h:70](../../../include/flashvm/flashvm.h#L70) |
| `fvm_get_global` | function | [include/flashvm/value.h:125](../../../include/flashvm/value.h#L125) |
| `fvm_set_global` | function | [include/flashvm/value.h:129](../../../include/flashvm/value.h#L129) |
| `fvm_value_number` | function | [include/flashvm/value.h:50](../../../include/flashvm/value.h#L50) |
| `fvm_value_string` | function | [include/flashvm/value.h:58](../../../include/flashvm/value.h#L58) |
| `fvm_worker_destroy` | function | [include/flashvm/worker.h:98](../../../include/flashvm/worker.h#L98) |
| `fvm_worker_join` | function | [include/flashvm/worker.h:60](../../../include/flashvm/worker.h#L60) |
| `fvm_worker_run` | function | [include/flashvm/worker.h:25](../../../include/flashvm/worker.h#L25) |
<!-- source-links:end -->
