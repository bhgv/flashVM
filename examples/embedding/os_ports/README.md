# embedding/os_ports

Использует native mouse polling и TCP connect/send/close. Показывает платформенный слой и ресурсное закрытие; наличие успешного TCP-соединения зависит от внешнего адреса и окружения.

## Реализация и запуск

Пример C/C++ собирается с `FVM_BUILD_EXAMPLES=ON`. Из корня репозитория:

```sh
cmake -S . -B build-examples -DFVM_DEFS_FILE=cmake/profiles/minimal.defs -DFVM_BUILD_EXAMPLES=ON
cmake --build build-examples --target flashvm_examples_embedding_os_ports_main --parallel
./build-examples/flashvm_examples_embedding_os_ports_main
```

Операции зависят от выбранного DEFS-профиля; OS_PORTS, JIT, графика и codecs
не включаются наличием этого README. Исходники ниже показывают порядок API-вызовов
и освобождения ресурсов.

<!-- example-catalog-api:start -->
**Текущая CMake-цель:** `flashvm_examples_embedding_os_ports_main`.


## Демонстрируемые функции C API

Имена ведут к публичным декларациям. Во второй колонке указан файл руководства
и раздел с контрактом; общие функции setup/cleanup включены для воспроизводимости.

| Функция | Где читать подробнее |
| --- | --- |
| [`fvm_port_mouse_poll`](../../../ports/input_port.h) | [OS_PORTS.md](../../../doc/OS_PORTS.md) — Платформенный ввод и TCP; доступность backend. |

[Описание в общем каталоге](../../README.md#example-embedding-os_ports).
<!-- example-catalog-api:end -->
