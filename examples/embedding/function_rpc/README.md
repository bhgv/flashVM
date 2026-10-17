# Original function interfaces: processes, threads and LAN

[Русский](README.ru.md) · [Source](main.c)

The C89 tutorial in `examples/embedding/function_rpc/` uses the same native
function in three arrangements. The original function and userdata remain in
the source VM. The receiving VM holds a callable interface. All calls use the
broker's one TCP listening port and carry routing/call IDs.

Requirements: CMake and C/C++ compilers supporting C89/C++98. The minimal
profile enables native sockets/threads, disables graphics/media and builds
bundled dependencies from the repository. Python 3 is needed for the automated
process check. Files: `main.c` is the tutorial; `function_rpc_example.h` contains
its constants.

Run these commands from the FlashVM repository root, not this example directory:


```sh
cmake -S . -B build-rpc -DFVM_DEFS_FILE=cmake/profiles/minimal.defs -DCMAKE_BUILD_TYPE=Release
cmake --build build-rpc --config Release --target flashvm_examples_embedding_function_rpc_main
```

On Windows, use the generated `.exe` and its configuration directory when the
selected generator creates one; arguments are the same.

## Two independent FlashVM runtimes on different threads

```sh
./build-rpc/flashvm_examples_embedding_function_rpc_main threads
```

The source runtime hands its context exclusively to the broker service thread.
`fvm_worker_run` creates a second independent runtime and context on a worker
thread. Only a primitive URI string is copied into the worker. It imports the
interface and invokes the original over TCP, even in one process. A VM object,
function address or userdata pointer is never passed to the recipient VM.
After joining the worker, the host reclaims the source context and checks its
original call counter. Output includes `two independent runtime threads via TCP
passed` and `original calls=4`.

## Two independent processes on one computer

Start the source in terminal A:

```sh
./build-rpc/flashvm_examples_embedding_function_rpc_main serve
```

Copy its first output line (the complete `fvm://...` URI). In terminal B:

```sh
./build-rpc/flashvm_examples_embedding_function_rpc_main call "URI_FROM_TERMINAL_A"
```

The client verifies result 42, the three-byte `C\0E` exception, unsupported and
ordinary error returns. After the client finishes, press Enter in terminal A.
The source reclaims its heap, checks four original userdata calls and shuts down.
No native callback serialization codec is registered in either process.

## Two computers connected by LAN

Build the example on both computers. If computer A has LAN address
`192.168.1.10`, run on A:

```sh
./build-rpc/flashvm_examples_embedding_function_rpc_main serve 0.0.0.0 192.168.1.10 9210
```

`0.0.0.0` selects local listening interfaces; `192.168.1.10` is the address
published inside the URI, reachable by computer B. To listen only on the LAN
interface, use `192.168.1.10` for both arguments. A reachable DNS name is also
accepted as the advertised host. TCP port 9210 must be reachable from B to A.
The printed URI is a bearer capability: give it only to the intended client.

On computer B, copy A's complete URI into:

```sh
./build-rpc/flashvm_examples_embedding_function_rpc_main call "URI_PRINTED_BY_COMPUTER_A"
```

The same four results are checked. Then press Enter on A to reclaim the source
context and stop. Replace the sample LAN address/port with the actual values;
never advertise `127.0.0.1` or `0.0.0.0` to another computer. All original
functions share the selected listening port; clients use ordinary outbound TCP
connections, not additional listening ports. Port 0 requests automatic port
selection; the URI contains the actual selected port.

Local automated checks cover independent VM threads, independent processes,
wildcard/interface binds, IPv4/DNS advertised hosts and malformed ports. They do
not certify execution on two physical computers or on foreign operating systems.
See [ownership and limitations](../../../doc/REMOTE_FUNCTIONS.md); cross-authority callback
cycles, persistent socket multiplexing and unrestricted concurrent heap access
remain open.

## Automated checks

After building, run from the repository root:

```sh
ctest --test-dir build-rpc -C Release -R "function_independent|examples_embedding_function_rpc_main" --output-on-failure
```

This covers usage, independent VM threads, separate processes, listening/
advertised addresses and invalid ports. The process test is registered when
CMake finds Python 3; the thread test does not require Python. Keep
`FVM_BUILD_EXAMPLES` and `FVM_BUILD_TESTS` enabled for this command.

<!-- example-catalog-api:start -->
## Назначение и особенности

Одна native-функция используется через интерфейсы между процессами, независимыми VM-потоками и узлами LAN. Показывает брокер, маршрутизацию вызовов, обслуживание исходного контекста, ошибки и время жизни экспортированной функции.

**Текущая CMake-цель:** `flashvm_examples_embedding_function_rpc_main`.


## Демонстрируемые функции C API

Имена ведут к публичным декларациям. Во второй колонке указан файл руководства
и раздел с контрактом; общие функции setup/cleanup включены для воспроизводимости.

| Функция | Где читать подробнее |
| --- | --- |
| [`fvm_broker_export`](../../../include/flashvm/capability.h) | [RESOURCE_TRANSPORT.md#русский](../../../doc/RESOURCE_TRANSPORT.md#русский) — Брокер, экспорт и импорт общих ресурсов. |
| [`fvm_broker_import`](../../../include/flashvm/capability.h) | [RESOURCE_TRANSPORT.md#русский](../../../doc/RESOURCE_TRANSPORT.md#русский) — Брокер, экспорт и импорт общих ресурсов. |
| [`fvm_broker_release`](../../../include/flashvm/capability.h) | [RESOURCE_TRANSPORT.md#русский](../../../doc/RESOURCE_TRANSPORT.md#русский) — Брокер, экспорт и импорт общих ресурсов. |
| [`fvm_broker_start`](../../../include/flashvm/capability.h) | [RESOURCE_TRANSPORT.md#русский](../../../doc/RESOURCE_TRANSPORT.md#русский) — Брокер, экспорт и импорт общих ресурсов. |
| [`fvm_broker_stop`](../../../include/flashvm/capability.h) | [RESOURCE_TRANSPORT.md#русский](../../../doc/RESOURCE_TRANSPORT.md#русский) — Брокер, экспорт и импорт общих ресурсов. |
| [`fvm_config_init`](../../../include/flashvm/flashvm.h) | [API_INDEX.md#runtime-and-context](../../../doc/API_INDEX.md#runtime-and-context) — Жизненный цикл runtime/context; точные сигнатуры и ownership — в заголовке. |
| [`fvm_context_create`](../../../include/flashvm/flashvm.h) | [API_INDEX.md#runtime-and-context](../../../doc/API_INDEX.md#runtime-and-context) — Жизненный цикл runtime/context; точные сигнатуры и ownership — в заголовке. |
| [`fvm_context_destroy`](../../../include/flashvm/flashvm.h) | [API_INDEX.md#runtime-and-context](../../../doc/API_INDEX.md#runtime-and-context) — Жизненный цикл runtime/context; точные сигнатуры и ownership — в заголовке. |
| [`fvm_context_function_serve`](../../../include/flashvm/capability.h) | [REMOTE_FUNCTIONS.md](../../../doc/REMOTE_FUNCTIONS.md) — Обслуживание удалённых вызовов на VM-потоке. |
| [`fvm_context_function_transfer`](../../../include/flashvm/capability.h) | [REMOTE_FUNCTIONS.md](../../../doc/REMOTE_FUNCTIONS.md) — Разрешение переноса интерфейсов функций. |
| [`fvm_context_set_broker`](../../../include/flashvm/capability.h) | [RESOURCE_TRANSPORT.md#русский](../../../doc/RESOURCE_TRANSPORT.md#русский) — Брокер, экспорт и импорт общих ресурсов. |
| [`fvm_exception_clear`](../../../include/flashvm/exception.h) | [STANDARD_ERROR.md](../../../doc/STANDARD_ERROR.md) — Ошибки и сохранение исходного исключения. |
| [`fvm_exception_get`](../../../include/flashvm/exception.h) | [STANDARD_ERROR.md](../../../doc/STANDARD_ERROR.md) — Ошибки и сохранение исходного исключения. |
| [`fvm_exception_raise`](../../../include/flashvm/exception.h) | [STANDARD_ERROR.md](../../../doc/STANDARD_ERROR.md) — Ошибки и сохранение исходного исключения. |
| [`fvm_function_create_native`](../../../include/flashvm/function.h) | [AVM1_FUNCTIONS.md](../../../doc/AVM1_FUNCTIONS.md) — Создание функций, аргументы, вызов и receiver. |
| [`fvm_function_invoke`](../../../include/flashvm/function.h) | [AVM1_FUNCTIONS.md](../../../doc/AVM1_FUNCTIONS.md) — Создание функций, аргументы, вызов и receiver. |
| [`fvm_gc_collect`](../../../include/flashvm/gc.h) | [GARBAGE_COLLECTION.md#api](../../../doc/GARBAGE_COLLECTION.md#api) — Корни, pin/unpin, режимы и ручная сборка. |
| [`fvm_gc_pin`](../../../include/flashvm/gc.h) | [GARBAGE_COLLECTION.md#api](../../../doc/GARBAGE_COLLECTION.md#api) — Корни, pin/unpin, режимы и ручная сборка. |
| [`fvm_runtime_create`](../../../include/flashvm/flashvm.h) | [API_INDEX.md#runtime-and-context](../../../doc/API_INDEX.md#runtime-and-context) — Жизненный цикл runtime/context; точные сигнатуры и ownership — в заголовке. |
| [`fvm_runtime_destroy`](../../../include/flashvm/flashvm.h) | [API_INDEX.md#runtime-and-context](../../../doc/API_INDEX.md#runtime-and-context) — Жизненный цикл runtime/context; точные сигнатуры и ownership — в заголовке. |
| [`fvm_serialized_free`](../../../include/flashvm/serialization.h) | [RESOURCE_TRANSPORT.md#русский](../../../doc/RESOURCE_TRANSPORT.md#русский) — Сериализация и освобождение буферов. |
| [`fvm_value_number`](../../../include/flashvm/value.h) | [API_INDEX.md#values-and-borrowed-views](../../../doc/API_INDEX.md#values-and-borrowed-views) — Типы значений, borrowed-данные и преобразования. |
| [`fvm_value_string`](../../../include/flashvm/value.h) | [API_INDEX.md#values-and-borrowed-views](../../../doc/API_INDEX.md#values-and-borrowed-views) — Типы значений, borrowed-данные и преобразования. |
| [`fvm_value_string_bytes`](../../../include/flashvm/value.h) | [API_INDEX.md#values-and-borrowed-views](../../../doc/API_INDEX.md#values-and-borrowed-views) — Типы значений, borrowed-данные и преобразования. |
| [`fvm_value_string_size`](../../../include/flashvm/value.h) | [API_INDEX.md#values-and-borrowed-views](../../../doc/API_INDEX.md#values-and-borrowed-views) — Типы значений, borrowed-данные и преобразования. |
| [`fvm_worker_current`](../../../include/flashvm/worker.h) | [WORKERS.md](../../../doc/WORKERS.md) — Изолированные worker-контексты и join. |
| [`fvm_worker_destroy`](../../../include/flashvm/worker.h) | [WORKERS.md](../../../doc/WORKERS.md) — Изолированные worker-контексты и join. |
| [`fvm_worker_join`](../../../include/flashvm/worker.h) | [WORKERS.md](../../../doc/WORKERS.md) — Изолированные worker-контексты и join. |
| [`fvm_worker_run`](../../../include/flashvm/worker.h) | [WORKERS.md](../../../doc/WORKERS.md) — Изолированные worker-контексты и join. |

[Описание в общем каталоге](../../README.md#example-embedding-function_rpc).
<!-- example-catalog-api:end -->
