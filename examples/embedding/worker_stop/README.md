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
| [`fvm_config_init`](../../../include/flashvm/flashvm.h) | [API_INDEX.md#runtime-and-context](../../../doc/API_INDEX.md#runtime-and-context) — Жизненный цикл runtime/context; точные сигнатуры и ownership — в заголовке. |
| [`fvm_value_number`](../../../include/flashvm/value.h) | [API_INDEX.md#values-and-borrowed-views](../../../doc/API_INDEX.md#values-and-borrowed-views) — Типы значений, borrowed-данные и преобразования. |
| [`fvm_worker_destroy`](../../../include/flashvm/worker.h) | [WORKERS.md](../../../doc/WORKERS.md) — Изолированные worker-контексты и join. |
| [`fvm_worker_join_all`](../../../include/flashvm/worker.h) | [WORKER_CANCELLATION.md](../../../doc/WORKER_CANCELLATION.md) — Ожидание завершения группы worker. |
| [`fvm_worker_run_suspended`](../../../include/flashvm/worker.h) | [WORKER_LIFECYCLE.md](../../../doc/WORKER_LIFECYCLE.md) — Создание worker без немедленного запуска. |
| [`fvm_worker_stop`](../../../include/flashvm/worker.h) | [WORKER_CANCELLATION.md](../../../doc/WORKER_CANCELLATION.md) — Кооперативная остановка worker. |
| [`fvm_worker_stop_requested`](../../../include/flashvm/worker.h) | [WORKER_CANCELLATION.md](../../../doc/WORKER_CANCELLATION.md) — Проверка запроса остановки. |

[Описание в общем каталоге](../../README.md#example-embedding-worker_stop).
<!-- example-catalog-api:end -->
