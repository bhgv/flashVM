# embedding/worker_stop

Запрашивает кооперативную остановку приостановленного worker и ожидает завершения через join_all. Показывает проверку stop_requested в callback и безопасное завершение без принудительного убийства потока.

## Реализация и запуск

Пример C/C++ собирается с `FVM_BUILD_EXAMPLES=ON`. Из корня репозитория:

```sh
cmake -S . -B build-examples -DFVM_DEFS_FILE=cmake/profiles/minimal.defs -DFVM_BUILD_EXAMPLES=ON
cmake --build build-examples --target flashvm_examples_embedding_worker_stop_main --parallel
./build-examples/flashvm_examples_embedding_worker_stop_main
```

Операции зависят от выбранного DEFS-профиля; OS_PORTS, JIT, графика и codecs
не включаются наличием этого README. Исходники ниже показывают порядок API-вызовов
и освобождения ресурсов.

<!-- example-catalog-api:start -->
**Текущая CMake-цель:** `flashvm_examples_embedding_worker_stop_main`.


## Демонстрируемые функции C API

Имена ведут к публичным декларациям. Во второй колонке указан файл руководства
и раздел с контрактом; общие функции setup/cleanup включены для воспроизводимости.

| Функция | Где читать подробнее |
| --- | --- |
| [`fvm_config_init`](../../../include/flashvm/flashvm.h#L62) | [API_INDEX.md#runtime-and-context](../../../doc/API_INDEX.md#runtime-and-context) — Жизненный цикл runtime/context; точные сигнатуры и ownership — в заголовке. |
| [`fvm_value_number`](../../../include/flashvm/value.h#L42) | [API_INDEX.md#values-and-borrowed-views](../../../doc/API_INDEX.md#values-and-borrowed-views) — Типы значений, borrowed-данные и преобразования. |
| [`fvm_worker_destroy`](../../../include/flashvm/worker.h#L71) | [WORKERS.md](../../../doc/WORKERS.md) — Изолированные worker-контексты и join. |
| [`fvm_worker_join_all`](../../../include/flashvm/worker.h#L60) | [WORKER_CANCELLATION.md](../../../doc/WORKER_CANCELLATION.md) — Ожидание завершения группы worker. |
| [`fvm_worker_run_suspended`](../../../include/flashvm/worker.h#L26) | [WORKER_LIFECYCLE.md](../../../doc/WORKER_LIFECYCLE.md) — Создание worker без немедленного запуска. |
| [`fvm_worker_stop`](../../../include/flashvm/worker.h#L52) | [WORKER_CANCELLATION.md](../../../doc/WORKER_CANCELLATION.md) — Кооперативная остановка worker. |
| [`fvm_worker_stop_requested`](../../../include/flashvm/worker.h#L54) | [WORKER_CANCELLATION.md](../../../doc/WORKER_CANCELLATION.md) — Проверка запроса остановки. |

[Описание в общем каталоге](../../README.md#example-embedding-worker_stop).
<!-- example-catalog-api:end -->

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_config_init` | function | [include/flashvm/flashvm.h:62](../../../include/flashvm/flashvm.h#L62) |
| `fvm_value_number` | function | [include/flashvm/value.h:42](../../../include/flashvm/value.h#L42) |
| `fvm_worker_destroy` | function | [include/flashvm/worker.h:71](../../../include/flashvm/worker.h#L71) |
| `fvm_worker_join_all` | function | [include/flashvm/worker.h:60](../../../include/flashvm/worker.h#L60) |
| `fvm_worker_run_suspended` | function | [include/flashvm/worker.h:26](../../../include/flashvm/worker.h#L26) |
| `fvm_worker_stop` | function | [include/flashvm/worker.h:52](../../../include/flashvm/worker.h#L52) |
| `fvm_worker_stop_requested` | function | [include/flashvm/worker.h:54](../../../include/flashvm/worker.h#L54) |
<!-- source-links:end -->
