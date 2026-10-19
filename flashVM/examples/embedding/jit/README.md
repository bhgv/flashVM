# embedding/jit

Явно компилирует тело x*2+2 и подключает его к обычной AVM1-функции. Сравнивает результат вызова и размер кода, показывает отдельное владение [FVM_JitCode](../../../src/jit/internal.hpp#L59) и interpreter fallback.

## Реализация и запуск

Пример C/C++ собирается с `FVM_BUILD_EXAMPLES=ON`. Из корня репозитория:

```sh
cmake -S . -B build-examples -DFVM_DEFS_FILE=cmake/profiles/minimal-jit.defs -DFVM_BUILD_EXAMPLES=ON
cmake --build build-examples --target flashvm_examples_embedding_jit_main --parallel
./build-examples/flashvm_examples_embedding_jit_main
```

Операции зависят от выбранного DEFS-профиля; OS_PORTS, JIT, графика и codecs
не включаются наличием этого README. Исходники ниже показывают порядок API-вызовов
и освобождения ресурсов.

<!-- example-catalog-api:start -->
**Текущая CMake-цель:** `flashvm_examples_embedding_jit_main`.


## Expected output / Ожидаемый вывод

On the current Linux x64 native test host:

```text
backend: x86-64 (compiled: yes)
interpreter: 42
native: 42 (N code bytes)
```

N depends on the backend/compiler. Windows and macOS Intel report windows-x64
and macos-x64 respectively. The example explicitly selects MANUAL before invocation, so the unbound first
call interprets even in a JIT-default build. It checks the native-entry counter
before and after attachment. Compiled availability describes the build; only the
native result line certifies that this invocation actually ran native code.
UNSUPPORTED compilation prints the explicit interpreter-retained line instead.
These names are not native Windows/macOS acceptance evidence from this workspace.

Число байтов зависит от backend. Строка compiled показывает доступность в сборке;
строка native появляется после реального вызова. При UNSUPPORTED сохраняется
интерпретатор. Пример не создаёт файлов и не меняет подписи/разрешения сам.

## Демонстрируемые функции C API

Имена ведут к публичным декларациям. Во второй колонке указан файл руководства
и раздел с контрактом; общие функции setup/cleanup включены для воспроизводимости.

| Функция | Где читать подробнее |
| --- | --- |
| [`fvm_execution_set_mode`](../../../include/flashvm/execution_mode.h#L64) | [EXECUTION_MODES.md](../../../doc/EXECUTION_MODES.md) — MANUAL interprets before binding and honors the explicit native attachment. |
| [`fvm_execution_get_stats`](../../../include/flashvm/execution_mode.h#L71) | [EXECUTION_MODES.md](../../../doc/EXECUTION_MODES.md) — Verify zero native entries before binding and a real native entry after it. |
| [`fvm_jit_available`](../../../include/flashvm/jit.h#L13) | [JIT_ARCHITECTURES.md](../../../doc/JIT_ARCHITECTURES.md) — Compiled ABI support, not per-mapping permission / наличие ABI в сборке. |
| [`fvm_jit_backend_name`](../../../include/flashvm/jit.h#L15) | [JIT_ARCHITECTURES.md](../../../doc/JIT_ARCHITECTURES.md) — Static selected ABI name / статическое имя ABI. |
| [`fvm_config_init`](../../../include/flashvm/flashvm.h#L62) | [API_INDEX.md#runtime-and-context](../../../doc/API_INDEX.md#runtime-and-context) — Жизненный цикл runtime/context; точные сигнатуры и ownership — в заголовке. |
| [`fvm_context_create`](../../../include/flashvm/flashvm.h#L68) | [API_INDEX.md#runtime-and-context](../../../doc/API_INDEX.md#runtime-and-context) — Жизненный цикл runtime/context; точные сигнатуры и ownership — в заголовке. |
| [`fvm_context_destroy`](../../../include/flashvm/flashvm.h#L70) | [API_INDEX.md#runtime-and-context](../../../doc/API_INDEX.md#runtime-and-context) — Жизненный цикл runtime/context; точные сигнатуры и ownership — в заголовке. |
| [`fvm_function_create_avm1`](../../../include/flashvm/function.h#L52) | [AVM1_FUNCTIONS.md](../../../doc/AVM1_FUNCTIONS.md) — Создание функций, аргументы, вызов и receiver. |
| [`fvm_function_invoke`](../../../include/flashvm/function.h#L65) | [AVM1_FUNCTIONS.md](../../../doc/AVM1_FUNCTIONS.md) — Создание функций, аргументы, вызов и receiver. |
| [`fvm_function_set_body`](../../../include/flashvm/function.h#L43) | [AVM1_FUNCTIONS.md](../../../doc/AVM1_FUNCTIONS.md) — Создание функций, аргументы, вызов и receiver. |
| [`fvm_jit_code_size`](../../../include/flashvm/jit.h#L35) | [DEBUG_API.md#fvm_jit_code_size](../../../doc/DEBUG_API.md#fvm_jit_code_size) — Параметры, статусы, время жизни и ограничения вызова. |
| [`fvm_jit_compile`](../../../include/flashvm/jit.h#L21) | [JIT.md#lifetime-gc-and-failures](../../../doc/JIT.md#lifetime-gc-and-failures) — JIT-код, исходное тело и время жизни. |
| [`fvm_jit_destroy`](../../../include/flashvm/jit.h#L28) | [JIT.md#lifetime-gc-and-failures](../../../doc/JIT.md#lifetime-gc-and-failures) — JIT-код, исходное тело и время жизни. |
| [`fvm_runtime_create`](../../../include/flashvm/flashvm.h#L64) | [API_INDEX.md#runtime-and-context](../../../doc/API_INDEX.md#runtime-and-context) — Жизненный цикл runtime/context; точные сигнатуры и ownership — в заголовке. |
| [`fvm_runtime_destroy`](../../../include/flashvm/flashvm.h#L66) | [API_INDEX.md#runtime-and-context](../../../doc/API_INDEX.md#runtime-and-context) — Жизненный цикл runtime/context; точные сигнатуры и ownership — в заголовке. |
| [`fvm_standard_install`](../../../include/flashvm/standard.h#L20) | [STANDARD_OBJECTS.md](../../../doc/STANDARD_OBJECTS.md) — Стандартные классы и объектная модель. |
| [`fvm_value_number`](../../../include/flashvm/value.h#L42) | [API_INDEX.md#values-and-borrowed-views](../../../doc/API_INDEX.md#values-and-borrowed-views) — Типы значений, borrowed-данные и преобразования. |

[Описание в общем каталоге](../../README.md#example-embedding-jit).
<!-- example-catalog-api:end -->


Windows x64 offline call-plan encoder and actual Linux `ms_abi` validation / автономный генератор и проверка ABI: [JIT_WINDOWS_X64.md](../../../doc/JIT_WINDOWS_X64.md). v0.561 adds Windows x64 selection, W^X allocation and registered unwind metadata; native Windows execution is still unverified. / Добавлены выбор backend, W^X и unwind; нативный Windows запуск ещё не проверен.


macOS Intel offline encoder and linked-frame validation / автономный генератор и цепочка кадров: [JIT_MACOS_X64.md](../../../doc/JIT_MACOS_X64.md). v0.564 adds classic desktop macOS x64 RW→RX publication and explicit signing preparation; native macOS acceptance remains pending / добавлены classic RW→RX и подготовка подписи, нативная приёмка ещё не выполнена.


## macOS Intel host preparation / Подготовка macOS Intel

Use [JIT_MACOS_X64.md](../../../doc/JIT_MACOS_X64.md#build-and-example--сборка-и-пример)
for x86_64 CMake commands and the classic RW→RX host policy. The optional
[macos-intel.entitlements](macos-intel.entitlements) plist contains the unsigned
executable-memory exception for a host that deliberately selects this policy.
It is not applied automatically. Hardened Runtime allow-jit/MAP_JIT and Apple
Silicon use a different policy and are not implemented by this adapter. Native
macOS execution and signing are not validated in this Linux workspace.

В руководстве приведены команды x86_64 сборки и необязательной локальной подписи.
Plist относится к classic Intel unsigned code; не включает Apple Silicon/MAP_JIT.
Подпись и разрешения host задаёт явно; библиотека не меняет их сама. Результат и
публичные compile/attach/invoke/source/destroy API примера остаются прежними.
Offline Darwin ARM64 call encoding is documented in [JIT_MACOS_ARM64.md](../../../doc/JIT_MACOS_ARM64.md). v0.569 adds an opt-in macOS 11.4+ MAP_JIT callback arena; default activation remains OFF pending native acceptance.
## macOS ARM64 host preparation / подготовка Apple Silicon

For the opt-in macOS 11.4+ MAP_JIT callback adapter, use the native build/sign/run
commands in [JIT_MACOS_ARM64.md](../../../doc/JIT_MACOS_ARM64.md#v0569-opt-in-macos-114-publication--включаемая-публикация).
The accompanying [macos-arm64.entitlements](macos-arm64.entitlements) enables
allow-jit and the callback allowlist for this executable. It is not the Intel
unsigned-memory entitlement file. Link statically at startup; the library does
not sign the application or coordinate callbacks loaded later through dlopen.
The tutorial's existing MANUAL/native-entry checks apply when this backend is
actually available. The build option defaults OFF pending native acceptance.

Нужен настоящий macOS ARM64 хост, SDK с callback API и deployment target 11.4+.
Файл entitlement и точные команды приведены в руководстве. Здесь подтверждены
эмуляция инструкций и controlled SDK protocol; нативная Apple-приёмка ещё не
выполнена. Отказ защиты/подписи должен оставаться ошибкой или interpreter fallback.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_config_init` | function | [include/flashvm/flashvm.h:62](../../../include/flashvm/flashvm.h#L62) |
| `fvm_context_create` | function | [include/flashvm/flashvm.h:68](../../../include/flashvm/flashvm.h#L68) |
| `fvm_context_destroy` | function | [include/flashvm/flashvm.h:70](../../../include/flashvm/flashvm.h#L70) |
| `fvm_execution_get_stats` | function | [include/flashvm/execution_mode.h:71](../../../include/flashvm/execution_mode.h#L71) |
| `fvm_execution_set_mode` | function | [include/flashvm/execution_mode.h:64](../../../include/flashvm/execution_mode.h#L64) |
| `fvm_function_create_avm1` | function | [include/flashvm/function.h:52](../../../include/flashvm/function.h#L52) |
| `fvm_function_invoke` | function | [include/flashvm/function.h:65](../../../include/flashvm/function.h#L65) |
| `fvm_function_set_body` | function | [include/flashvm/function.h:43](../../../include/flashvm/function.h#L43) |
| `fvm_jit_available` | function | [include/flashvm/jit.h:13](../../../include/flashvm/jit.h#L13) |
| `fvm_jit_backend_name` | function | [include/flashvm/jit.h:15](../../../include/flashvm/jit.h#L15) |
| `fvm_jit_code_size` | function | [include/flashvm/jit.h:35](../../../include/flashvm/jit.h#L35) |
| `fvm_jit_compile` | function | [include/flashvm/jit.h:21](../../../include/flashvm/jit.h#L21) |
| `fvm_jit_destroy` | function | [include/flashvm/jit.h:28](../../../include/flashvm/jit.h#L28) |
| `FVM_JitCode` | struct | [src/jit/internal.hpp:59](../../../src/jit/internal.hpp#L59) |
| `fvm_runtime_create` | function | [include/flashvm/flashvm.h:64](../../../include/flashvm/flashvm.h#L64) |
| `fvm_runtime_destroy` | function | [include/flashvm/flashvm.h:66](../../../include/flashvm/flashvm.h#L66) |
| `fvm_standard_install` | function | [include/flashvm/standard.h:20](../../../include/flashvm/standard.h#L20) |
| `fvm_value_number` | function | [include/flashvm/value.h:42](../../../include/flashvm/value.h#L42) |
<!-- source-links:end -->
