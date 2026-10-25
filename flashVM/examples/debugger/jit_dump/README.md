# JIT-код и ZIP-дамп / JIT source and ZIP

Демонстрирует связь native-handle с исходным AVM1, просмотр read-only машинных байтов и создание ZIP со снимком VM и активным JIT-кодом.

## Что происходит

На поддерживаемом ABI исходное AVM1-тело компилируется явно, проверяются [fvm_jit_source](../../../include/flashvm/jit.h#L43) и [fvm_jit_machine_code](../../../include/flashvm/jit.h#L47), затем handle подключается к обычной script-функции. MANUAL выбран явно, чтобы использовать это подключение. Если backend отсутствует, остаётся интерпретация и ZIP без машинного раздела.

## Реализация и время жизни

ZIP_CODE сохраняет raw globals/объекты/классы, available locals, AVM1-тела и фактические активные native mapping. Проверяются отдельно исходный VM-результат и diagnostic_status: ошибка записи файла не считается успешным дампом. Код освобождается после уничтожения контекста, когда все подключения к нему уже исчезли.

Общие helpers [`../support.h`](../support.h) создают isolated runtime/context,
устанавливают стандартные классы, печатают точные коды ошибок и освобождают
контекст раньше runtime. Их setup/cleanup APIs перечислены в
[общем README](../README.md#общая-инициализация-и-ошибки). Каждая непустая строка
C/header прокомментирована; проверки выполняются обычным control flow.

## Сборка и запуск

Из корня репозитория:

```sh
cmake -S . -B build-debugger -DFVM_DEFS_FILE="$PWD/cmake/profiles/minimal.defs" \
  -DFVM_BUILD_TESTS=OFF -DFVM_BUILD_EXAMPLES=ON -DCMAKE_BUILD_TYPE=Release
cmake --build build-debugger --target flashvm_examples_debugger_jit_dump_main --parallel
./build-debugger/flashvm_examples_debugger_jit_dump_main
```

## Ожидаемый результат

Печатаются размер исходника, backend/размер native mapping и имя ZIP. По умолчанию создаётся debugger-jit.zip в текущем каталоге; единственный аргумент задаёт другой путь. Существующий файл по этому пути может быть перезаписан.

## Ограничения и особенности

ZIP — диагностический снимок, а не восстанавливаемый образ VM. В нём нет raw памяти внешних native-ресурсов и состояния удалённого процесса. SIGSEGV/SEH не перехватываются. Идентичность исходного RX-адреса нельзя разыменовывать после удаления handle.

NATURAL/FORCE — политика VM, независимая от CMake Release/Debug или host
компиляторных -g/-O0. Host callbacks и borrowed pointers принадлежат указанным
в API владельцам. Любая неожиданная ошибка даёт ненулевой exit code.

## Используемые функции

- [`fvm_crash_configure`](../../../doc/DEBUG_API.md#fvm_crash_configure) — подробный debugger/runtime-контракт.
- [`fvm_crash_data`](../../../doc/DEBUG_API.md#fvm_crash_data) — подробный debugger/runtime-контракт.
- [`fvm_crash_info`](../../../doc/DEBUG_API.md#fvm_crash_info) — подробный debugger/runtime-контракт.
- [`fvm_exception_clear`](../../../include/flashvm/exception.h#L74) — декларация и контракт общего VM API.
- [`fvm_execution_set_mode`](../../../doc/DEBUG_API.md#fvm_execution_set_mode) — подробный debugger/runtime-контракт.
- [`fvm_function_create_avm1`](../../../include/flashvm/function.h#L62) — декларация и контракт общего VM API.
- [`fvm_function_invoke`](../../../include/flashvm/function.h#L79) — декларация и контракт общего VM API.
- [`fvm_function_set_body`](../../../include/flashvm/function.h#L49) — декларация и контракт общего VM API.
- [`fvm_jit_available`](../../../doc/DEBUG_API.md#fvm_jit_available) — подробный debugger/runtime-контракт.
- [`fvm_jit_backend_name`](../../../doc/DEBUG_API.md#fvm_jit_backend_name) — подробный debugger/runtime-контракт.
- [`fvm_jit_body`](../../../doc/JIT.md) — декларация и контракт общего VM API.
- [`fvm_jit_compile`](../../../doc/JIT.md) — декларация и контракт общего VM API.
- [`fvm_jit_destroy`](../../../doc/JIT.md) — декларация и контракт общего VM API.
- [`fvm_jit_machine_code`](../../../doc/DEBUG_API.md#fvm_jit_machine_code) — подробный debugger/runtime-контракт.
- [`fvm_jit_source`](../../../doc/DEBUG_API.md#fvm_jit_source) — подробный debugger/runtime-контракт.

[Полный справочник EN](../../../doc/DEBUG_API.md) ·
[Справочник RU](../../../doc/DEBUG_API.ru.md) ·
[Политики сбоев](../../../doc/CRASH_DIAGNOSTICS.md) ·
[SWF/SWD](../../../doc/SWD_DEBUGGING.md).

## English

This C89 tutorial demonstrates **JIT source and ZIP**. It uses explicit
error paths and line-by-line comments, with no process assertions. Build/run
commands above select the minimal profile. The implementation/lifetime notes
explain the expected VM result, borrowed data and cleanup; every used FlashVM
function links to its detailed reference or public ownership contract.

<!-- example-catalog-api:start -->
## Назначение и особенности

Явно компилирует AVM1, связывает машинное тело с исходным байткодом и создаёт ZIP_CODE-дамп. Проверяет результат записи отдельно от исходного исключения; при отсутствии JIT демонстрирует интерпретаторный вариант.

**Текущая CMake-цель:** `flashvm_examples_debugger_jit_dump_main`.


## Демонстрируемые функции C API

Имена ведут к публичным декларациям. Во второй колонке указан файл руководства
и раздел с контрактом; общие функции setup/cleanup включены для воспроизводимости.

| Функция | Где читать подробнее |
| --- | --- |
| [`fvm_crash_configure`](../../../include/flashvm/crash.h#L65) | [DEBUG_API.md#fvm_crash_configure](../../../doc/DEBUG_API.md#fvm_crash_configure) — Параметры, статусы, время жизни и ограничения вызова. |
| [`fvm_crash_data`](../../../include/flashvm/crash.h#L98) | [DEBUG_API.md#fvm_crash_data](../../../doc/DEBUG_API.md#fvm_crash_data) — Параметры, статусы, время жизни и ограничения вызова. |
| [`fvm_crash_info`](../../../include/flashvm/crash.h#L81) | [DEBUG_API.md#fvm_crash_info](../../../doc/DEBUG_API.md#fvm_crash_info) — Параметры, статусы, время жизни и ограничения вызова. |
| [`fvm_exception_clear`](../../../include/flashvm/exception.h#L74) | [STANDARD_ERROR.md](../../../doc/STANDARD_ERROR.md) — Ошибки и сохранение исходного исключения. |
| [`fvm_execution_set_mode`](../../../include/flashvm/execution_mode.h#L79) | [DEBUG_API.md#fvm_execution_set_mode](../../../doc/DEBUG_API.md#fvm_execution_set_mode) — Параметры, статусы, время жизни и ограничения вызова. |
| [`fvm_function_create_avm1`](../../../include/flashvm/function.h#L62) | [AVM1_FUNCTIONS.md](../../../doc/AVM1_FUNCTIONS.md) — Создание функций, аргументы, вызов и receiver. |
| [`fvm_function_invoke`](../../../include/flashvm/function.h#L79) | [AVM1_FUNCTIONS.md](../../../doc/AVM1_FUNCTIONS.md) — Создание функций, аргументы, вызов и receiver. |
| [`fvm_function_set_body`](../../../include/flashvm/function.h#L49) | [AVM1_FUNCTIONS.md](../../../doc/AVM1_FUNCTIONS.md) — Создание функций, аргументы, вызов и receiver. |
| [`fvm_jit_available`](../../../include/flashvm/jit.h#L15) | [DEBUG_API.md#fvm_jit_available](../../../doc/DEBUG_API.md#fvm_jit_available) — Параметры, статусы, время жизни и ограничения вызова. |
| [`fvm_jit_backend_name`](../../../include/flashvm/jit.h#L19) | [DEBUG_API.md#fvm_jit_backend_name](../../../doc/DEBUG_API.md#fvm_jit_backend_name) — Параметры, статусы, время жизни и ограничения вызова. |
| [`fvm_jit_compile`](../../../include/flashvm/jit.h#L27) | [JIT.md#lifetime-gc-and-failures](../../../doc/JIT.md#lifetime-gc-and-failures) — JIT-код, исходное тело и время жизни. |
| [`fvm_jit_destroy`](../../../include/flashvm/jit.h#L38) | [JIT.md#lifetime-gc-and-failures](../../../doc/JIT.md#lifetime-gc-and-failures) — JIT-код, исходное тело и время жизни. |
| [`fvm_jit_machine_code`](../../../include/flashvm/jit.h#L47) | [DEBUG_API.md#fvm_jit_machine_code](../../../doc/DEBUG_API.md#fvm_jit_machine_code) — Параметры, статусы, время жизни и ограничения вызова. |
| [`fvm_jit_source`](../../../include/flashvm/jit.h#L43) | [DEBUG_API.md#fvm_jit_source](../../../doc/DEBUG_API.md#fvm_jit_source) — Параметры, статусы, время жизни и ограничения вызова. |

[Описание в общем каталоге](../../README.md#example-debugger-jit_dump).
<!-- example-catalog-api:end -->

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_crash_configure` | function | [include/flashvm/crash.h:65](../../../include/flashvm/crash.h#L65) |
| `fvm_crash_data` | function | [include/flashvm/crash.h:98](../../../include/flashvm/crash.h#L98) |
| `fvm_crash_info` | function | [include/flashvm/crash.h:81](../../../include/flashvm/crash.h#L81) |
| `fvm_exception_clear` | function | [include/flashvm/exception.h:74](../../../include/flashvm/exception.h#L74) |
| `fvm_execution_set_mode` | function | [include/flashvm/execution_mode.h:79](../../../include/flashvm/execution_mode.h#L79) |
| `fvm_function_create_avm1` | function | [include/flashvm/function.h:62](../../../include/flashvm/function.h#L62) |
| `fvm_function_invoke` | function | [include/flashvm/function.h:79](../../../include/flashvm/function.h#L79) |
| `fvm_function_set_body` | function | [include/flashvm/function.h:49](../../../include/flashvm/function.h#L49) |
| `fvm_jit_available` | function | [include/flashvm/jit.h:15](../../../include/flashvm/jit.h#L15) |
| `fvm_jit_backend_name` | function | [include/flashvm/jit.h:19](../../../include/flashvm/jit.h#L19) |
| `fvm_jit_body` | function | [include/flashvm/jit.h:32](../../../include/flashvm/jit.h#L32) |
| `fvm_jit_compile` | function | [include/flashvm/jit.h:27](../../../include/flashvm/jit.h#L27) |
| `fvm_jit_destroy` | function | [include/flashvm/jit.h:38](../../../include/flashvm/jit.h#L38) |
| `fvm_jit_machine_code` | function | [include/flashvm/jit.h:47](../../../include/flashvm/jit.h#L47) |
| `fvm_jit_source` | function | [include/flashvm/jit.h:43](../../../include/flashvm/jit.h#L43) |
<!-- source-links:end -->
