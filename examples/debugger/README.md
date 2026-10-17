# Примеры debugger API / debugger tutorials

Отдельные C89-программы показывают разные способы использовать отладчик внутри
embedding-приложения. UI не требуется. Каждый пример имеет подробный README,
обычные проверки ошибок, явное освобождение ресурсов и комментарии каждой
непустой строки C/header. Сборка автоматически входит в FVM_BUILD_EXAMPLES.

| Пример | Возможность |
| --- | --- |
| [backtrace](backtrace/README.md) | Сохранённый бэктрейс |
| [live_variables](live_variables/README.md) | Живые переменные и FORCE |
| [jit_dump](jit_dump/README.md) | JIT-код и ZIP-дамп |
| [source_breakpoints](source_breakpoints/README.md) | SWF/SWD и текущая строка |
| [channel_inspection](channel_inspection/README.md) | Перехват сообщений канала |
| [resource_snapshots](resource_snapshots/README.md) | Реестры VM и владение mutex |

## Общая инициализация и ошибки

[`support.h`](support.h) содержит читаемые setup/check/cleanup helpers.
`debugger_ok` печатает точный FVM_Result и возвращает boolean; он не завершает
процесс. `debugger_open` возвращает частично созданные handles, поэтому cleanup
обязателен даже при setup failure. `debugger_close` освобождает context раньше
runtime после возврата всех синхронных callback/call.

Общие используемые API:

- [`fvm_config_init`, `fvm_runtime_create`, `fvm_runtime_destroy`, `fvm_context_create`, `fvm_context_destroy`](../../include/flashvm/flashvm.h) — инициализация ABI и ownership.
- [`fvm_standard_install`](../../include/flashvm/standard.h) — стандартные классы для инспекции.
- [`fvm_function_create_avm1`, `fvm_function_invoke`, `fvm_function_set_body`](../../include/flashvm/function.h) — обычные управляемые функции/подключения.
- [`fvm_value_number`](../../include/flashvm/value.h) — заполнение typed input без allocation/coercion.
- [`fvm_exception_clear`](../../include/flashvm/exception.h) — отдельная обработка намеренного script throw.

Каждый README дополнительно перечисляет именно свои debugger APIs со ссылками
на [полный справочник](../../doc/DEBUG_API.md) и [RU-справочник](../../doc/DEBUG_API.ru.md).
Обычные функции stdio используются для консоли и host-файлов; malloc/free — только
для caller-owned копий снимков в примере ресурсов.

## Границы примеров

Ожидаемый FVM_EXCEPTION в примере означает сознательно выполненный Throw;
программа возвращает host status 0 только после успешной инспекции и обработки.
NATURAL использует существующее состояние. FORCE явно добавляет сохранение
аргументов/строк и register-параметров; он не отключает настоящий JIT.
CMake Debug/-g относится к host C/C++ и не заменяет runtime detail policy.

ZIP-пример пишет debugger-jit.zip в текущий каталог и допускает явный другой
путь. Это диагностический архив VM, а не архив исходников проекта. Другие
примеры работают в памяти. Source tutorial использует маленький matching SWF/SWD
fixture; реальный host передаёт свои файловые буферы в тот же load_swd API.

Source/line mapping и callbacks не устанавливают signal/SEH handler, не чинят
повреждённую память и не обещают безопасно продолжить аварийную native
инструкцию. Подробные границы: [CRASH_DIAGNOSTICS](../../doc/CRASH_DIAGNOSTICS.md)
и [SWD_DEBUGGING](../../doc/SWD_DEBUGGING.md).

## English

Six standalone C89 tutorials cover retained backtraces, live forced inspection,
source/native ZIPs, matching SWF/SWD source breakpoints, nondestructive channel
inspection and coherent resource/registry snapshots. Every nonblank C/header
line is commented. Each README documents purpose, behavior, implementation,
ownership, build/run commands, limits and function-reference links.
