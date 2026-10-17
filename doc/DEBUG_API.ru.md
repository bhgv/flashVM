# Справочник C API отладчика

C89-декларации: `flashvm/debug.h`, `crash.h`, `source_debug.h`, `exception.h`,
`jit.h`, `execution_mode.h`. Ниже документированы все публичные функции
debug/crash/source inspection и используемые учебными примерами JIT/runtime-
переключатели. Общие API создания VM, объектов и каналов описаны в своих headers.

Операции контекста выполняются на его VM-потоке. Снимки реестров ресурсов имеют
собственные process-lock, но не приостанавливают чужие контексты. Заимствованные
значения/строки надо копировать до указанной границы инвалидирования. В fault-
callback нельзя запускать скрипт, вызывать GC, изменять или уничтожать VM.
Breakpoint-callback дополнительно допускает изменение breakpoint и будущего
режима исполнения. Возврат из fault-callback продолжает раскрутку ошибки,
а не повтор аварийной инструкции. API не является signal-safe обработчиком ОС.

[Примеры examples/debugger](../examples/debugger/README.md) используют обычные
проверки ошибок и комментарии каждой строки. Общие форматы и ограничения:
[диагностика](CRASH_DIAGNOSTICS.md), [SWF/SWD](SWD_DEBUGGING.md).
Для точного сопоставления с C ABI имена параметров и коды ниже сохранены;
полные технические детали приведены для каждой функции.

## Index / Указатель

- [fvm_debug_set_breakpoint](#fvm_debug_set_breakpoint) — Устанавливает или снимает breakpoint по имени функции и смещению AVM1.
- [fvm_debug_set_breakpoint_handler](#fvm_debug_set_breakpoint_handler) — Регистрирует синхронный callback остановки интерпретатора.
- [fvm_debug_set_location](#fvm_debug_set_location) — Задаёт имя исходника и строку для последующих диагностик ресурсов.
- [fvm_debug_map_action](#fvm_debug_map_action) — Копирует ручное соответствие функция/AVM1-смещение → исходник/строка.
- [fvm_debug_mutex_snapshot](#fvm_debug_mutex_snapshot) — Копирует согласованный снимок владельцев/ожидающих мьютексов.
- [fvm_debug_mutex_detail_snapshot](#fvm_debug_mutex_detail_snapshot) — Копирует согласованный снимок владельцев/ожидающих мьютексов с адресами endpoint.
- [fvm_debug_channel_freeze](#fvm_debug_channel_freeze) — Приостанавливает или возобновляет доставку, не останавливая производителей.
- [fvm_debug_channel_state](#fvm_debug_channel_state) — Копирует состояние очереди/файла/удалённого endpoint без получения сообщения.
- [fvm_debug_channel_message](#fvm_debug_channel_message) — Декодирует независимый снимок непрочитанного сообщения без изъятия из очереди.
- [fvm_debug_channel_wait_snapshot](#fvm_debug_channel_wait_snapshot) — Копирует текущие ожидания ActionScript-операций каналов.
- [fvm_debug_resource_errors](#fvm_debug_resource_errors) — Копирует ограниченную очередь событий, от старых к новым.
- [fvm_debug_exception_events](#fvm_debug_exception_events) — Копирует ограниченную очередь событий, от старых к новым.
- [fvm_debug_object_count](#fvm_debug_object_count) — Возвращает число управляемых объектов VM.
- [fvm_debug_object_at](#fvm_debug_object_at) — Читает запись реестра по индексу с нуля.
- [fvm_debug_native_function_count](#fvm_debug_native_function_count) — Возвращает число записей глобального реестра native-функций.
- [fvm_debug_native_function_at](#fvm_debug_native_function_at) — Читает запись реестра по индексу с нуля.
- [fvm_debug_native_function_at_bytes](#fvm_debug_native_function_at_bytes) — Читает полное имя записи как указатель и длину в байтах.
- [fvm_debug_script_function_count](#fvm_debug_script_function_count) — Возвращает число записей реестра именованных script-функций.
- [fvm_debug_script_function_at](#fvm_debug_script_function_at) — Читает запись реестра по индексу с нуля.
- [fvm_debug_script_function_at_bytes](#fvm_debug_script_function_at_bytes) — Читает полное имя записи как указатель и длину в байтах.
- [fvm_crash_configure](#fvm_crash_configure) — Выбирает действие при first-chance ошибке и необязательный путь/callback.
- [fvm_crash_set_detail](#fvm_crash_set_detail) — Выбирает стоимость сохранения независимо от действия при ошибке.
- [fvm_crash_clear](#fvm_crash_clear) — Освобождает сохранённый отчёт, сохраняя выбранные политики.
- [fvm_crash_info](#fvm_crash_info) — Раздельно возвращает исходную ошибку и результат построения/записи отчёта.
- [fvm_crash_frame](#fvm_crash_frame) — Читает скопированный postmortem-кадр, начиная с самого внутреннего.
- [fvm_crash_frame_engine](#fvm_crash_frame_engine) — Различает интерпретатор, сгенерированный JIT и подключённое native-тело хоста.
- [fvm_crash_source_line](#fvm_crash_source_line) — Читает скопированный текст выполнявшейся строки из SWD.
- [fvm_crash_data](#fvm_crash_data) — Читает сохранённый JSON-бэктрейс или ZIP как указатель и длину.
- [fvm_crash_argument](#fvm_crash_argument) — Читает живой аргумент вызова во время остановки callback.
- [fvm_crash_local](#fvm_crash_local) — Читает материализованную локальную переменную во время остановки callback.
- [fvm_crash_register](#fvm_crash_register) — Читает живой ABI-регистр во время остановки callback.
- [fvm_debug_load_swd](#fvm_debug_load_swd) — Атомарно подключает соответствующий SWF/SWD-набор исходников.
- [fvm_debug_clear_symbols](#fvm_debug_clear_symbols) — Удаляет текущий набор символов и исходников.
- [fvm_debug_source_count](#fvm_debug_source_count) — Возвращает число файлов исходников в подключённом SWD.
- [fvm_debug_source_at](#fvm_debug_source_at) — Читает ID, имя и полный текст одного встроенного исходника.
- [fvm_debug_source_position](#fvm_debug_source_position) — Сопоставляет копию AVM1-тела и относительное смещение с исходной строкой.
- [fvm_debug_current_source](#fvm_debug_current_source) — Определяет исходную строку текущей выполняемой AVM1-инструкции.
- [fvm_debug_register_name](#fvm_debug_register_name) — Определяет имя существующего регистра по SWD-метаданным локальных переменных.
- [fvm_jit_source](#fvm_jit_source) — Читает исходный AVM1-байткод native-handle.
- [fvm_jit_machine_code](#fvm_jit_machine_code) — Читает байты сгенерированного RX-тела native-handle.
- [fvm_jit_available](#fvm_jit_available) — Проверяет доступность native-компилятора и ABI текущего хоста.
- [fvm_jit_backend_name](#fvm_jit_backend_name) — Возвращает статическое имя backend/ABI.
- [fvm_jit_code_size](#fvm_jit_code_size) — Возвращает суммарный размер native-кода, включая дочерние блоки.
- [fvm_execution_options_init](#fvm_execution_options_init) — Инициализирует явные опции с режимом интерпретатора.
- [fvm_execution_set_mode](#fvm_execution_set_mode) — Меняет политику следующих входов в тела.
- [fvm_execution_get_options](#fvm_execution_get_options) — Читает запрошенную политику исполнения.
- [fvm_execution_set_logger](#fvm_execution_set_logger) — Регистрирует синхронный logger диагностического replay.
- [fvm_execution_get_stats](#fvm_execution_get_stats) — Копирует счётчики исполнения, кеша и replay.
- [fvm_avm1_execute_options](#fvm_avm1_execute_options) — Применяет опции перед исполнением корневого AVM1-тела.
- [fvm_function_invoke_options](#fvm_function_invoke_options) — Применяет опции перед обычным вызовом функции.
- [fvm_debug_frame_count](#fvm_debug_frame_count) — Возвращает число заимствованных активаций во время живой остановки.
- [fvm_debug_frame_at](#fvm_debug_frame_at) — Читает живой кадр функции без построения postmortem-отчёта.

## fvm_debug_set_breakpoint

Устанавливает или снимает breakpoint по имени функции и смещению AVM1.

Declaration / декларация: [`flashvm/debug.h`](../include/flashvm/debug.h).

**Parameters / параметры.** `context`: owning VM. `function`: terminated name; empty selects root actions. `offset`: byte offset inside the action body. `enabled`: exactly 0 or 1.

**Results / результаты.** `OK`: installed/removed or already in the requested state. `INVALID_ARGUMENT`: null/invalid context/name, destruction/GC-release phase or invalid enabled value. `OUT_OF_MEMORY`: allocating a new name/list failed.

**Lifetime / время жизни.** Function name is copied. No code/VM handle ownership is transferred.

**Behavior / поведение.** Every same-named body is excluded from JIT on its next entry, including an explicit JIT attachment. Running native frames are not patched or deoptimized. An arbitrary non-action offset can be registered but never hits; removing a missing point succeeds.


## fvm_debug_set_breakpoint_handler

Регистрирует синхронный callback остановки интерпретатора.

Declaration / декларация: [`flashvm/debug.h`](../include/flashvm/debug.h).

**Parameters / параметры.** `context`: owning VM. `handler`: callback or null to remove it. `userdata`: borrowed host state passed unchanged.

**Results / результаты.** `OK` on registration; `INVALID_ARGUMENT` for a null, destroying or GC-releasing context.

**Lifetime / время жизни.** Handler/userdata remain host-owned until replaced or context destruction. The location and its strings are callback-only views.

**Behavior / поведение.** The handler returns OK to continue interpretation, another status to stop this VM call. It may change breakpoints or future execution mode; it must not execute script, collect or destroy the context. Callbacks are recursion-suppressed. A source-aware stop can call fvm_debug_current_source. Host callbacks that throw C++ exceptions are converted to OOM/ERROR.


## fvm_debug_set_location

Задаёт имя исходника и строку для последующих диагностик ресурсов.

Declaration / декларация: [`flashvm/debug.h`](../include/flashvm/debug.h).

**Parameters / параметры.** `context`: owning VM. `source`: terminated host source name. `line`: host line number, zero if unknown.

**Results / результаты.** `OK`; `INVALID_ARGUMENT` for null inputs/destroying context; `OUT_OF_MEMORY` if copying source fails.

**Lifetime / время жизни.** The source label is copied. This creates no file loader or source-content ownership.

**Behavior / поведение.** This changes current host location metadata; it does not add an action map or a breakpoint. Use fvm_debug_map_action for persistent body-offset mappings and load_swd for actual source contents.


## fvm_debug_map_action

Копирует ручное соответствие функция/AVM1-смещение → исходник/строка.

Declaration / декларация: [`flashvm/debug.h`](../include/flashvm/debug.h).

**Parameters / параметры.** `context`: owning VM. `function`: terminated name, empty for root. `offset`: relative action offset. `source`: terminated source label. `line`: mapped line.

**Results / результаты.** `OK`; `INVALID_ARGUMENT` for null inputs/destruction; `OUT_OF_MEMORY` for copied metadata.

**Lifetime / время жизни.** Names and source labels are copied into context-owned mapping storage.

**Behavior / поведение.** The nearest preceding mapping for the same function applies. Registering the same function/offset replaces its mapping. Same-named bodies share manual maps; manual report maps take precedence over SWD fallback. It does not store source text.


## fvm_debug_mutex_snapshot

Копирует согласованный снимок владельцев/ожидающих мьютексов.

Declaration / декларация: [`flashvm/debug.h`](../include/flashvm/debug.h).

**Parameters / параметры.** `out`: caller-owned FVM_DebugMutex array, or null only with capacity zero. `capacity`: row capacity. `count`: required nonnull output for total needed rows.

**Results / результаты.** `OK` if all rows fit; `NOT_FOUND` if capacity is insufficient (count is still set, rows unchanged); `INVALID_ARGUMENT` for missing count or null array with nonzero capacity.

**Lifetime / время жизни.** All fields/strings are copied under the process registry lock; no borrowed mutex pointers escape. Caller owns output storage.

**Behavior / поведение.** Probe with null/zero, allocate, retry if another thread grows the registry. There is one row per waiter, or one waiter=0 row for an unawaited handle. No network request occurs. Deadlock fields describe the local wait graph; they do not certify a remote/distributed cycle. Names/endpoints use bounded struct arrays and may be truncated.


## fvm_debug_mutex_detail_snapshot

Копирует согласованный снимок владельцев/ожидающих мьютексов с адресами endpoint.

Declaration / декларация: [`flashvm/debug.h`](../include/flashvm/debug.h).

**Parameters / параметры.** `out`: caller-owned FVM_DebugMutexDetail array, or null only with capacity zero. `capacity`: row capacity. `count`: required nonnull output for total needed rows.

**Results / результаты.** `OK` if all rows fit; `NOT_FOUND` if capacity is insufficient (count is still set, rows unchanged); `INVALID_ARGUMENT` for missing count or null array with nonzero capacity.

**Lifetime / время жизни.** All fields/strings are copied under the process registry lock; no borrowed mutex pointers escape. Caller owns output storage.

**Behavior / поведение.** Probe with null/zero, allocate, retry if another thread grows the registry. There is one row per waiter, or one waiter=0 row for an unawaited handle. No network request occurs. Deadlock fields describe the local wait graph; they do not certify a remote/distributed cycle. Names/endpoints use bounded struct arrays and may be truncated.


## fvm_debug_channel_freeze

Приостанавливает или возобновляет доставку, не останавливая производителей.

Declaration / декларация: [`flashvm/debug.h`](../include/flashvm/debug.h).

**Parameters / параметры.** `channel`: valid retained queue/file/compatible remote endpoint. `frozen`: zero to resume; any nonzero value freezes.

**Results / результаты.** `OK`; `INVALID_ARGUMENT` for null channel; remote transport/protocol failures propagate, including unsupported older broker controls.

**Lifetime / время жизни.** The handle remains caller-owned; keep it alive through the synchronous call.

**Behavior / поведение.** Local queues retain unread messages while frozen; local files pause reads and preserve cursors. Sending/writing remains possible. A closed frozen queue still needs resume before draining. Compatible remote brokers control authority-side freezing, but a remotely closed endpoint still rejects receive. Do not destroy concurrently used handles.


## fvm_debug_channel_state

Копирует состояние очереди/файла/удалённого endpoint без получения сообщения.

Declaration / декларация: [`flashvm/debug.h`](../include/flashvm/debug.h).

**Parameters / параметры.** `channel`: retained endpoint. `out`: required FVM_DebugChannel output struct (identity, pending, capacity, closed, frozen).

**Results / результаты.** `OK`; `INVALID_ARGUMENT` for null inputs; transport, malformed framing, scan-budget or file-position errors otherwise. Failed scans leave output unchanged.

**Lifetime / время жизни.** The result is a copied observation, not a reservation or delivery guarantee.

**Behavior / поведение.** Local queue counters are read under its lock. A file reports complete unread framed records, capacity=0 and restores its cursor after scanning (65536 records / 64 MiB work). Closed file pending=0. Remote identity names the local proxy and remote counters are Uint32. Concurrent producers/consumers may change state immediately after return.


## fvm_debug_channel_message

Декодирует независимый снимок непрочитанного сообщения без изъятия из очереди.

Declaration / декларация: [`flashvm/debug.h`](../include/flashvm/debug.h).

**Parameters / параметры.** `context`: destination VM on its owning thread. `channel`: retained endpoint. `index`: zero-based unread message index. `out`: required value output.

**Results / результаты.** `OK`; `NOT_FOUND` for unavailable indexed messages; `INVALID_ARGUMENT` for invalid inputs; OOM, serialization/resource/protocol/closed/scan failures propagate. Failed decoding leaves output Undefined.

**Lifetime / время жизни.** Decoded managed values belong to context. Pin returned object graphs before any later GC. Remote capabilities remain live shared resources.

**Behavior / поведение.** File peeks restore the read cursor and have the same bounded scan policy as state inspection. This freezes neither live resource state nor an external process. Registered graph codecs/resource identity rules still apply. Inspection is synchronous and can perform remote transport; do not call it from a crash callback that forbids mutation/allocation of VM state.


## fvm_debug_channel_wait_snapshot

Копирует текущие ожидания ActionScript-операций каналов.

Declaration / декларация: [`flashvm/debug.h`](../include/flashvm/debug.h).

**Parameters / параметры.** `out`: caller-owned FVM_DebugChannelWait rows, nullable only with capacity zero. `capacity`: maximum rows. `count`: required needed-row output.

**Results / результаты.** `OK` if all rows fit; `NOT_FOUND` if capacity is insufficient (count updated, rows and queue unchanged); `INVALID_ARGUMENT` for invalid array/count arguments.

**Lifetime / время жизни.** Copied rows contain bounded source/function/address strings and numeric identities; no live VM/handle ownership is transferred.

**Behavior / поведение.** This is a coherent process registry observation of actual script send/receive waits. It does not infer channel deadlocks or enumerate every host blocking operation.


## fvm_debug_resource_errors

Копирует ограниченную очередь событий, от старых к новым.

Declaration / декларация: [`flashvm/debug.h`](../include/flashvm/debug.h).

**Parameters / параметры.** `out`: caller-owned FVM_DebugResourceError rows, nullable only with capacity zero. `capacity`: maximum rows. `count`: required needed-row output. `clear`: nonzero removes the copied batch only after successful full copying.

**Results / результаты.** `OK` if all rows fit; `NOT_FOUND` if capacity is insufficient (count updated, rows and queue unchanged); `INVALID_ARGUMENT` for invalid array/count arguments.

**Lifetime / время жизни.** Copied rows contain bounded source/function/address strings and numeric identities; no live VM/handle ownership is transferred.

**Behavior / поведение.** The process-wide queue retains at most 64 events and drops the oldest on overflow. Probe/allocate/copy may need retry if new events arrive. clear=0 is nondestructive. Resource events are coarse intercept records (result can be FVM_ERROR); inspect structured exception info for richer fault codes.


## fvm_debug_exception_events

Копирует ограниченную очередь событий, от старых к новым.

Declaration / декларация: [`flashvm/exception.h`](../include/flashvm/exception.h).

**Parameters / параметры.** `out`: caller-owned FVM_ExceptionInfo rows, nullable only with capacity zero. `capacity`: maximum rows. `count`: required needed-row output. `clear`: nonzero removes the copied batch only after successful full copying.

**Results / результаты.** `OK` if all rows fit; `NOT_FOUND` if capacity is insufficient (count updated, rows and queue unchanged); `INVALID_ARGUMENT` for invalid array/count arguments.

**Lifetime / время жизни.** Copied rows contain bounded source/function/address strings and numeric identities; no live VM/handle ownership is transferred.

**Behavior / поведение.** The process-wide queue retains at most 64 events and drops the oldest on overflow. Probe/allocate/copy may need retry if new events arrive. clear=0 is nondestructive. This queue records structured resource exceptions, not every arbitrary script throw; use crash first-chance reports for script faults.


## fvm_debug_object_count

Возвращает число управляемых объектов VM.

Declaration / декларация: [`flashvm/debug.h`](../include/flashvm/debug.h).

**Parameters / параметры.** `context`: owning VM. `out`: required size_t count output.

**Results / результаты.** `OK`; `INVALID_ARGUMENT` if either input is null.

**Lifetime / время жизни.** No ownership is transferred and the count is not a frozen snapshot across subsequent VM mutations.

**Behavior / поведение.** Call on the VM thread while collection/execution mutation is stopped. Heap enumeration includes classes/instances; use ordinary table/debug inspection cautiously and pin retained objects before GC.


## fvm_debug_object_at

Читает запись реестра по индексу с нуля.

Declaration / декларация: [`flashvm/debug.h`](../include/flashvm/debug.h).

**Parameters / параметры.** `context`: owning VM. `index`: zero-based row below its current count. `out`: required FVM_Object ** object output.

**Results / результаты.** `OK`; `INVALID_ARGUMENT` for null arguments or an out-of-range index (legacy APIs do not use NOT_FOUND for end).

**Lifetime / время жизни.** The object is borrowed; enumeration does not pin it. Pin before retaining across GC.

**Behavior / поведение.** Use the corresponding _count result under a stable owning-thread observation. Numeric object indices can move after collection. Registry order is implementation storage order, not lexical sorting or a stable ID.


## fvm_debug_native_function_count

Возвращает число записей глобального реестра native-функций.

Declaration / декларация: [`flashvm/debug.h`](../include/flashvm/debug.h).

**Parameters / параметры.** `context`: owning VM. `out`: required size_t count output.

**Results / результаты.** `OK`; `INVALID_ARGUMENT` if either input is null.

**Lifetime / время жизни.** No ownership is transferred and the count is not a frozen snapshot across subsequent VM mutations.

**Behavior / поведение.** Call on the VM thread while collection/execution mutation is stopped. This is a specific registry, not every callable closure or first-class function in the heap. Borrowed registry keys can be invalidated by registry mutation.


## fvm_debug_native_function_at

Читает запись реестра по индексу с нуля.

Declaration / декларация: [`flashvm/debug.h`](../include/flashvm/debug.h).

**Parameters / параметры.** `context`: owning VM. `index`: zero-based row below its current count. `out`: required const char ** terminated registry-name output.

**Results / результаты.** `OK`; `INVALID_ARGUMENT` for null arguments or an out-of-range index (legacy APIs do not use NOT_FOUND for end).

**Lifetime / время жизни.** The terminated name is borrowed until registry mutation/context destruction. Use the counted _at_bytes form for embedded NUL and complete byte identity.

**Behavior / поведение.** Use the corresponding _count result under a stable owning-thread observation. Numeric object indices can move after collection. Registry order is implementation storage order, not lexical sorting or a stable ID.


## fvm_debug_native_function_at_bytes

Читает полное имя записи как указатель и длину в байтах.

Declaration / декларация: [`flashvm/debug.h`](../include/flashvm/debug.h).

**Parameters / параметры.** `context`: owning VM. `index`: current zero-based registry row. `out`: required const char ** name. `size`: required byte-count output.

**Results / результаты.** `OK`; `INVALID_ARGUMENT` for null arguments or an out-of-range index.

**Lifetime / время жизни.** The byte view is borrowed until registry mutation/context destruction; it is not a newly allocated string.

**Behavior / поведение.** Preserves embedded NUL and long names. Prefer this form in debugger UIs, copying the bytes if inspection will continue after mutation. Native and script registries remain distinct from heap function objects.


## fvm_debug_script_function_count

Возвращает число записей реестра именованных script-функций.

Declaration / декларация: [`flashvm/debug.h`](../include/flashvm/debug.h).

**Parameters / параметры.** `context`: owning VM. `out`: required size_t count output.

**Results / результаты.** `OK`; `INVALID_ARGUMENT` if either input is null.

**Lifetime / время жизни.** No ownership is transferred and the count is not a frozen snapshot across subsequent VM mutations.

**Behavior / поведение.** Call on the VM thread while collection/execution mutation is stopped. This is a specific registry, not every callable closure or first-class function in the heap. Borrowed registry keys can be invalidated by registry mutation.


## fvm_debug_script_function_at

Читает запись реестра по индексу с нуля.

Declaration / декларация: [`flashvm/debug.h`](../include/flashvm/debug.h).

**Parameters / параметры.** `context`: owning VM. `index`: zero-based row below its current count. `out`: required const char ** terminated registry-name output.

**Results / результаты.** `OK`; `INVALID_ARGUMENT` for null arguments or an out-of-range index (legacy APIs do not use NOT_FOUND for end).

**Lifetime / время жизни.** The terminated name is borrowed until registry mutation/context destruction. Use the counted _at_bytes form for embedded NUL and complete byte identity.

**Behavior / поведение.** Use the corresponding _count result under a stable owning-thread observation. Numeric object indices can move after collection. Registry order is implementation storage order, not lexical sorting or a stable ID.


## fvm_debug_script_function_at_bytes

Читает полное имя записи как указатель и длину в байтах.

Declaration / декларация: [`flashvm/debug.h`](../include/flashvm/debug.h).

**Parameters / параметры.** `context`: owning VM. `index`: current zero-based registry row. `out`: required const char ** name. `size`: required byte-count output.

**Results / результаты.** `OK`; `INVALID_ARGUMENT` for null arguments or an out-of-range index.

**Lifetime / время жизни.** The byte view is borrowed until registry mutation/context destruction; it is not a newly allocated string.

**Behavior / поведение.** Preserves embedded NUL and long names. Prefer this form in debugger UIs, copying the bytes if inspection will continue after mutation. Native and script registries remain distinct from heap function objects.


## fvm_crash_configure

Выбирает действие при first-chance ошибке и необязательный путь/callback.

Declaration / декларация: [`flashvm/crash.h`](../include/flashvm/crash.h).

**Parameters / параметры.** `context`: owning VM outside execution. `mode`: NONE, BACKTRACE, ZIP, ZIP_CODE or DEBUGGER. `path`: optional terminated ZIP output path, null for memory-only. `handler`: required for DEBUGGER, otherwise unused. `userdata`: borrowed host state, invalid without a handler.

**Results / результаты.** `OK`; `INVALID_ARGUMENT` for invalid enum/arguments, live execution, destruction, GC-release or active diagnostic callback; `OUT_OF_MEMORY` if copying path fails. Failed path copy leaves old policy intact.

**Lifetime / время жизни.** The path is copied. Callback/state remain host-owned until replacement or destruction. Configuration does not clear an older retained report.

**Behavior / поведение.** First-chance means an enclosing Try may later handle the failure. ZIP paths are overwritten only when that capture reaches file writing; capture failure can leave an older file. Check diagnostic_status. Debugger return continues error unwinding, not instruction retry. No signal/SEH handler is installed.


## fvm_crash_set_detail

Выбирает стоимость сохранения независимо от действия при ошибке.

Declaration / декларация: [`flashvm/crash.h`](../include/flashvm/crash.h).

**Parameters / параметры.** `context`: owning VM outside execution. `detail`: NATURAL or FORCE.

**Results / результаты.** `OK`; `INVALID_ARGUMENT` for invalid detail/context, live execution or an active callback/destruction/GC-release.

**Lifetime / время жизни.** Policy is context-owned; no argument values are copied by this configuration call.

**Behavior / поведение.** NATURAL borrows already rooted activation/argument state and adds no entry value copies or per-instruction local spills. FORCE copies supplied argument cells and strings at entry, adds GC roots and exposes formal register parameter names. Current JIT already materializes locals/registers and remains native. Object identity is retained, not a deep entry-time object graph snapshot; failed preservation is explicit in diagnostic_status/frame.forced.


## fvm_crash_clear

Освобождает сохранённый отчёт, сохраняя выбранные политики.

Declaration / декларация: [`flashvm/crash.h`](../include/flashvm/crash.h).

**Parameters / параметры.** `context`: required owning VM, outside a diagnostic callback.

**Results / результаты.** `OK`; `INVALID_ARGUMENT` for null context or active diagnostic callback.

**Lifetime / время жизни.** All previously borrowed frame/JSON/ZIP/source-line views become invalid immediately.

**Behavior / поведение.** This does not clear an ActionScript pending exception and does not delete an output file. Use fvm_exception_clear for exception handling and host file APIs for deliberate file removal.


## fvm_crash_info

Раздельно возвращает исходную ошибку и результат построения/записи отчёта.

Declaration / декларация: [`flashvm/crash.h`](../include/flashvm/crash.h).

**Parameters / параметры.** `context`: owning VM. `frames`: required size_t output. `status`: required original VM result output. `diagnostic_status`: required capture/preservation/I/O result output.

**Results / результаты.** `OK` when a report exists; `NOT_FOUND` with frames=0 when absent; `INVALID_ARGUMENT` for null arguments. Diagnostic status is written even when report construction failed; original status is only defined on OK.

**Lifetime / время жизни.** Copied scalar outputs remain valid independently of subsequent report replacement.

**Behavior / поведение.** A VM EXCEPTION and diagnostic OK is a successful report of a failing script. OOM or section-limit exhaustion reports diagnostic OUT_OF_MEMORY; file/callback errors can report ERROR without replacing the VM failure. A live DEBUGGER callback still runs after capture allocation failure.


## fvm_crash_frame

Читает скопированный postmortem-кадр, начиная с самого внутреннего.

Declaration / декларация: [`flashvm/crash.h`](../include/flashvm/crash.h).

**Parameters / параметры.** `context`: owning VM. `index`: retained frame index. `out`: required FVM_CrashFrame output struct.

**Results / результаты.** `OK`; `NOT_FOUND` if no report or index is beyond retained frames; `INVALID_ARGUMENT` for null context/output. Failed lookup leaves out unchanged.

**Lifetime / время жизни.** Name/source/source-bytecode/native-byte views belong to the report until clear/replacement/context destruction. They survive original function/code deletion and GC.

**Behavior / поведение.** offset is an AVM1 action offset, not a native PC. jit marks emitted code; query frame_engine to distinguish other engines. forced confirms entry preservation. machine_address is an opaque original RX address, never a pointer to dereference after code release; machine_code is a copied payload only for ZIP_CODE. Frames are bounded to 256 and JSON marks truncation.


## fvm_crash_frame_engine

Различает интерпретатор, сгенерированный JIT и подключённое native-тело хоста.

Declaration / декларация: [`flashvm/crash.h`](../include/flashvm/crash.h).

**Parameters / параметры.** `context`: owning VM. `index`: retained frame. `out`: required FVM_CrashEngine output.

**Results / результаты.** `OK`; `NOT_FOUND` for an absent report/frame; `INVALID_ARGUMENT` for null inputs.

**Lifetime / время жизни.** The returned enum is a copied scalar.

**Behavior / поведение.** An arbitrary FVM_ScriptBody attachment is NATIVE, while fvm_jit_body is JIT. Ordinary C stack frames/native callbacks are not a platform stack-unwind result. Native host bodies may have no exposed machine-byte view.


## fvm_crash_source_line

Читает скопированный текст выполнявшейся строки из SWD.

Declaration / декларация: [`flashvm/crash.h`](../include/flashvm/crash.h).

**Parameters / параметры.** `context`: owning VM. `index`: retained frame. `out`: required const char **. `size`: required byte-count output.

**Results / результаты.** `OK` for a known line, including an empty one; `NOT_FOUND` for missing report/frame/source resolution; `INVALID_ARGUMENT` for null arguments.

**Lifetime / время жизни.** The line copy belongs to the report and survives later clearing/replacing imported symbols.

**Behavior / поведение.** CR/LF terminators are excluded. A manual source label alone contains no source text. Source text is never fabricated from bytecode when an associated SWD is missing or ambiguous.


## fvm_crash_data

Читает сохранённый JSON-бэктрейс или ZIP как указатель и длину.

Declaration / декларация: [`flashvm/crash.h`](../include/flashvm/crash.h).

**Parameters / параметры.** `context`: owning VM. `zip`: zero for JSON, nonzero for ZIP. `out`: required const void **. `size`: required byte count.

**Results / результаты.** `OK` if a report exists, including size=0 when no ZIP was requested/built; `NOT_FOUND` if no report; `INVALID_ARGUMENT` for null arguments.

**Lifetime / время жизни.** The bytes are borrowed until report clear/replacement/destruction; they are not NUL-terminated strings and must not be freed by the caller.

**Behavior / поведение.** Check info.diagnostic_status before claiming a file or ZIP succeeded. JSON escapes preserve original bytes, numbers are strings and object identities are diagnostic-only. ZIP includes raw object/class fields, bytecode and optional machine/source sections, not a restorable VM image.


## fvm_crash_argument

Читает живой аргумент вызова во время остановки callback.

Declaration / декларация: [`flashvm/crash.h`](../include/flashvm/crash.h).

**Parameters / параметры.** `context`: live fault or breakpoint debugger callback VM. `frame`: innermost-first active frame index. `index`: zero-based supplied argument. `out`: required value output.

**Results / результаты.** `OK`; `NOT_FOUND` outside a fault/breakpoint callback or for an unavailable frame/index/bank; `INVALID_ARGUMENT` for null required inputs. Failed lookup leaves outputs unchanged.

**Lifetime / время жизни.** Value/object/string/name views are borrowed only until callback return. Do not collect, execute script, mutate or destroy context while using them.

**Behavior / поведение.** NATURAL sees the already supplied argument storage; FORCE sees preserved entry scalar/string values when frame.forced is true. Object contents are current, not deep entry snapshots. Original arguments are distinct from mutated parameter/register bindings.


## fvm_crash_local

Читает материализованную локальную переменную во время остановки callback.

Declaration / декларация: [`flashvm/crash.h`](../include/flashvm/crash.h).

**Parameters / параметры.** `context`: live fault or breakpoint debugger callback VM. `frame`: active frame. `index`: zero-based available local row. `name`/`name_size`: required counted-name outputs. `out`: required value output.

**Results / результаты.** `OK`; `NOT_FOUND` outside a fault/breakpoint callback or for an unavailable frame/index/bank; `INVALID_ARGUMENT` for null required inputs. Failed lookup leaves outputs unchanged.

**Lifetime / время жизни.** Value/object/string/name views are borrowed only until callback return. Do not collect, execute script, mutate or destroy context while using them.

**Behavior / поведение.** Enumerates raw activation properties without getters. FORCE additionally exposes materialized Function2 register parameters under formal names after raw locals; these are diagnostic aliases, not new ActionScript bindings. Anonymous register locals can be inspected separately and named through SWD. Root activations may have no local object.


## fvm_crash_register

Читает живой ABI-регистр во время остановки callback.

Declaration / декларация: [`flashvm/crash.h`](../include/flashvm/crash.h).

**Parameters / параметры.** `context`: live fault or breakpoint debugger callback VM. `frame`: active frame. `index`: ABI register slot. `out`: required value output.

**Results / результаты.** `OK`; `NOT_FOUND` outside a fault/breakpoint callback or for an unavailable frame/index/bank; `INVALID_ARGUMENT` for null required inputs. Failed lookup leaves outputs unchanged.

**Lifetime / время жизни.** Value/object/string/name views are borrowed only until callback return. Do not collect, execute script, mutate or destroy context while using them.

**Behavior / поведение.** Reads already-maintained private/global register storage. No value spill is forced by the query. Use SWD register_name for a known source name; absent names remain unavailable. Values are not retained after the callback.


## fvm_debug_load_swd

Атомарно подключает соответствующий SWF/SWD-набор исходников.

Declaration / декларация: [`flashvm/source_debug.h`](../include/flashvm/source_debug.h).

**Parameters / параметры.** `context`: owning VM outside execution. `swf`/`swf_size`: complete FWS/CWS input. `swd`/`swd_size`: complete FWD6--8 data. All spans must remain valid for this call.

**Results / результаты.** `OK`; `INVALID_ARGUMENT` for invalid state/spans, malformed/truncated records, duplicate modules, conflicting lines or DebugID mismatch; `NOT_FOUND` if SWF has no DebugID; `UNSUPPORTED` for unknown/version/work-limit records; `OUT_OF_MEMORY` on allocation. Failed import keeps the previous bundle.

**Lifetime / время жизни.** Inputs are copied/decoded. One bundle is owned per context; a successful new load replaces the prior bundle and invalidates borrowed source views.

**Behavior / поведение.** SWD SCRIPT/OFFSET/ID/REGISTERS/BREAKPOINT/end records are parsed; breakpoint records are validated but not auto-enabled. ID must exactly match the 16-byte root SWF DebugID. CWS offsets refer to canonical uncompressed bytes. Source paths are labels only: no file/network access or automatic source retrieval. Limit: 16 MiB SWD, 65536 records; source bundles stay process-local and are not graph-transferred.


## fvm_debug_clear_symbols

Удаляет текущий набор символов и исходников.

Declaration / декларация: [`flashvm/source_debug.h`](../include/flashvm/source_debug.h).

**Parameters / параметры.** `context`: owning VM outside execution.

**Results / результаты.** `OK`, including no previously loaded bundle; `INVALID_ARGUMENT` for null/destroying context or live execution/diagnostic callback.

**Lifetime / время жизни.** All borrowed source/module/name views expire. Copied crash source lines and ZIP data remain valid until their report is cleared.

**Behavior / поведение.** Does not erase manual action maps, existing breakpoints or the loaded movie. A new SWF can be associated only through an explicit matching load_swd call.


## fvm_debug_source_count

Возвращает число файлов исходников в подключённом SWD.

Declaration / декларация: [`flashvm/source_debug.h`](../include/flashvm/source_debug.h).

**Parameters / параметры.** `context`: owning VM. `count`: required size_t output.

**Results / результаты.** `OK` with zero when no bundle is loaded; `INVALID_ARGUMENT` for null inputs.

**Lifetime / время жизни.** The scalar count does not pin the bundle against a subsequent explicit replacement.

**Behavior / поведение.** Use source_at by ordinal to inspect module IDs, original filenames and complete embedded contents. No source files are opened.


## fvm_debug_source_at

Читает ID, имя и полный текст одного встроенного исходника.

Declaration / декларация: [`flashvm/source_debug.h`](../include/flashvm/source_debug.h).

**Parameters / параметры.** `context`: owning VM. `index`: zero-based module ordinal. `module_id`, `name`, `name_size`, `text`, `text_size`: required outputs.

**Results / результаты.** `OK`; `NOT_FOUND` without a bundle or beyond its module count; `INVALID_ARGUMENT` for null outputs/context.

**Lifetime / время жизни.** Names/content are borrowed counted byte views until symbol clear/replacement/context destruction; the module ID is a copied scalar.

**Behavior / поведение.** Text can be empty and may use CRLF/CR/LF. Keep byte lengths and decode text in the host UI as appropriate. This API shows the original supplied contents, not a decompiled substitute.


## fvm_debug_source_position

Сопоставляет копию AVM1-тела и относительное смещение с исходной строкой.

Declaration / декларация: [`flashvm/source_debug.h`](../include/flashvm/source_debug.h).

**Parameters / параметры.** `context`: owning VM. `body`/`body_size`: exact AVM1 body bytes (original, JIT view or crash copy). `offset`: relative byte offset below body size. `out`: required FVM_DebugSourcePosition.

**Results / результаты.** `OK`; `NOT_FOUND` for missing/unmapped/ambiguous source; `INVALID_ARGUMENT` for invalid spans/output/offset; `UNSUPPORTED` when bounded candidate/work limits are exceeded. Failed lookup leaves out unchanged.

**Lifetime / время жизни.** The returned source-name and line-text pointers belong to the attached bundle, not the body buffer; line excludes CR/LF.

**Behavior / поведение.** Lookup is cold and bounded, based on exact bytes in the DebugID-associated canonical SWF plus SWD absolute offsets. Identical bodies with conflicting locations are rejected instead of showing another function file. Limits: 4096 body candidates and 128 MiB compared work per query. No bytecode-to-source scan is added to the instruction hot path.


## fvm_debug_current_source

Определяет исходную строку текущей выполняемой AVM1-инструкции.

Declaration / декларация: [`flashvm/source_debug.h`](../include/flashvm/source_debug.h).

**Parameters / параметры.** `context`: owning VM during a breakpoint/debugger/native host callback. `out`: required FVM_DebugSourcePosition.

**Results / результаты.** `OK`; `NOT_FOUND` without an active source-bearing frame/bundle; lookup INVALID_ARGUMENT/UNSUPPORTED outcomes otherwise.

**Lifetime / время жизни.** Returned source views are borrowed from the symbol bundle; use them only while that bundle remains loaded.

**Behavior / поведение.** Both interpreted and JIT activations publish source associations when symbols are attached. Ordinary breakpoint callbacks receive SWD filename/line automatically on a hit. Lookup happens on inspection/hit, not every instruction. It does not asynchronously suspend another VM thread.


## fvm_debug_register_name

Определяет имя существующего регистра по SWD-метаданным локальных переменных.

Declaration / декларация: [`flashvm/source_debug.h`](../include/flashvm/source_debug.h).

**Parameters / параметры.** `context`: owning VM. `body`/`body_size`/`offset`: same exact-body query as source_position. `reg`: ABI register number. `name`/`name_size`: required counted-name outputs.

**Results / результаты.** `OK`; `NOT_FOUND` for absent/ambiguous names, body origins or out-of-scope records; INVALID_ARGUMENT/UNSUPPORTED for invalid/bounded lookup outcomes.

**Lifetime / время жизни.** The returned name is borrowed from the bundle until clear/replacement/destruction.

**Behavior / поведение.** Uses the latest applicable register declaration inside a unique matched body span. It neither materializes a missing value nor guesses unknown names. Pair with crash_register during a live stop. Register records from another identical body are rejected conservatively.


## fvm_jit_source

Читает исходный AVM1-байткод native-handle.

Declaration / декларация: [`flashvm/jit.h`](../include/flashvm/jit.h).

**Parameters / параметры.** `code`: valid unreleased FVM_JitCode. `out`: required const void **. `size`: required byte-count output.

**Results / результаты.** `OK`; `INVALID_ARGUMENT` for null handle/outputs. An invalid freed handle is a host ownership violation, not safely detectable input.

**Lifetime / время жизни.** Borrowed bytes live until fvm_jit_destroy and must not be written or freed. Source views are separate from copied report views.

**Behavior / поведение.** Source is the exact owned copy used for compilation; use it with source_position or host disassembly tooling.


## fvm_jit_machine_code

Читает байты сгенерированного RX-тела native-handle.

Declaration / декларация: [`flashvm/jit.h`](../include/flashvm/jit.h).

**Parameters / параметры.** `code`: valid unreleased FVM_JitCode. `out`: required const void **. `size`: required byte-count output.

**Results / результаты.** `OK`; `INVALID_ARGUMENT` for null handle/outputs. An invalid freed handle is a host ownership violation, not safely detectable input.

**Lifetime / время жизни.** Borrowed bytes live until fvm_jit_destroy and must not be written or freed. Source views are separate from copied report views.

**Behavior / поведение.** Mapping view excludes page padding and nested mappings. It identifies emitted bytes, not source-line/native-PC relocation tables. Active structured ZIP_CODE reports capture the actual failing child mapping.


## fvm_jit_available

Проверяет доступность native-компилятора и ABI текущего хоста.

Declaration / декларация: [`flashvm/jit.h`](../include/flashvm/jit.h).

**Parameters / параметры.** No arguments.

**Results / результаты.** Returns a boolean integer.

**Lifetime / время жизни.** No ownership transfer or allocation. Backend label is static process data.

**Behavior / поведение.** See [JIT ownership/compilation](JIT.md) for compile, attach, execute and destroy contracts. Availability is not a promise that every AVM1 body compiles or that executable allocation cannot later fail.


## fvm_jit_backend_name

Возвращает статическое имя backend/ABI.

Declaration / декларация: [`flashvm/jit.h`](../include/flashvm/jit.h).

**Parameters / параметры.** No arguments.

**Results / результаты.** Returns a static terminated label, including unsupported.

**Lifetime / время жизни.** No ownership transfer or allocation. Backend label is static process data.

**Behavior / поведение.** See [JIT ownership/compilation](JIT.md) for compile, attach, execute and destroy contracts. Availability is not a promise that every AVM1 body compiles or that executable allocation cannot later fail.


## fvm_jit_code_size

Возвращает суммарный размер native-кода, включая дочерние блоки.

Declaration / декларация: [`flashvm/jit.h`](../include/flashvm/jit.h).

**Parameters / параметры.** `code`: native handle, or null for zero.

**Results / результаты.** Returns a size_t byte count; zero for null.

**Lifetime / время жизни.** No ownership transfer or allocation. Backend label is static process data.

**Behavior / поведение.** See [JIT ownership/compilation](JIT.md) for compile, attach, execute and destroy contracts. Availability is not a promise that every AVM1 body compiles or that executable allocation cannot later fail.


## fvm_execution_options_init

Инициализирует явные опции с режимом интерпретатора.

Declaration / декларация: [`flashvm/execution_mode.h`](../include/flashvm/execution_mode.h).

**Parameters / параметры.** `out`: required options structure.

**Results / результаты.** `OK`; `INVALID_ARGUMENT` for null output.

**Lifetime / время жизни.** No new ownership transfer beyond the ordinary owning-context API; callbacks/userdata are borrowed and scalar outputs are copied.

**Behavior / поведение.** Sets struct_size, INTERPRETER and flags=0 independently of the DEFS context default. See [execution modes](EXECUTION_MODES.md).


## fvm_execution_set_mode

Меняет политику следующих входов в тела.

Declaration / декларация: [`flashvm/execution_mode.h`](../include/flashvm/execution_mode.h).

**Parameters / параметры.** `context`: owning VM. `mode`: MANUAL/INTERPRETER/JIT. `flags`: zero or REPLAY_ON_EXCEPTION.

**Results / результаты.** `OK`; `INVALID_ARGUMENT` for invalid mode/flags/context phase.

**Lifetime / время жизни.** No new ownership transfer beyond the ordinary owning-context API; callbacks/userdata are borrowed and scalar outputs are copied.

**Behavior / поведение.** Active native code is not patched. Interpreter bypasses fvm_jit_body attachments; arbitrary host bodies retain their own ABI. Replay can repeat side effects and is not rollback. See [execution modes](EXECUTION_MODES.md).


## fvm_execution_get_options

Читает запрошенную политику исполнения.

Declaration / декларация: [`flashvm/execution_mode.h`](../include/flashvm/execution_mode.h).

**Parameters / параметры.** `context`: owning VM. `out`: required options with struct_size initialized.

**Results / результаты.** `OK`; `INVALID_ARGUMENT` for invalid context/output/struct size.

**Lifetime / время жизни.** No new ownership transfer beyond the ordinary owning-context API; callbacks/userdata are borrowed and scalar outputs are copied.

**Behavior / поведение.** Shows requested policy even when diagnostic replay temporarily forces interpretation. See [execution modes](EXECUTION_MODES.md).


## fvm_execution_set_logger

Регистрирует синхронный logger диагностического replay.

Declaration / декларация: [`flashvm/execution_mode.h`](../include/flashvm/execution_mode.h).

**Parameters / параметры.** `context`: owning VM. `logger`: callback, or null for stderr. `userdata`: borrowed host state, invalid without a callback.

**Results / результаты.** `OK`; `INVALID_ARGUMENT` for invalid state/arguments.

**Lifetime / время жизни.** No new ownership transfer beyond the ordinary owning-context API; callbacks/userdata are borrowed and scalar outputs are copied.

**Behavior / поведение.** Logger may query stats/change future mode but must not execute script, collect, throw or destroy VM. Ordinary native execution has no per-instruction logger calls. See [execution modes](EXECUTION_MODES.md).


## fvm_execution_get_stats

Копирует счётчики исполнения, кеша и replay.

Declaration / декларация: [`flashvm/execution_mode.h`](../include/flashvm/execution_mode.h).

**Parameters / параметры.** `context`: owning VM. `out`: required stats with struct_size initialized.

**Results / результаты.** `OK`; `INVALID_ARGUMENT` for invalid context/output/struct size.

**Lifetime / время жизни.** No new ownership transfer beyond the ordinary owning-context API; callbacks/userdata are borrowed and scalar outputs are copied.

**Behavior / поведение.** Counters wrap according to unsigned long width. last_replay_status describes the retry, not the original returned exception. See [execution modes](EXECUTION_MODES.md).


## fvm_avm1_execute_options

Применяет опции перед исполнением корневого AVM1-тела.

Declaration / декларация: [`flashvm/execution_mode.h`](../include/flashvm/execution_mode.h).

**Parameters / параметры.** `context`, `data`, `size`: ordinary root-execution inputs. `options`: null to retain current policy, or initialized explicit options.

**Results / результаты.** Option validation errors occur before execution; ordinary AVM1 statuses then propagate.

**Lifetime / время жизни.** No new ownership transfer beyond the ordinary owning-context API; callbacks/userdata are borrowed and scalar outputs are copied.

**Behavior / поведение.** Options persist in the context after this call. Invalid options do not execute source. See [execution modes](EXECUTION_MODES.md).


## fvm_function_invoke_options

Применяет опции перед обычным вызовом функции.

Declaration / декларация: [`flashvm/execution_mode.h`](../include/flashvm/execution_mode.h).

**Parameters / параметры.** `context`, `function`, `receiver`, `argc`, `argv`, `out`: ordinary invocation inputs; `options`: optional initialized policy.

**Results / результаты.** Option validation errors occur before invocation; ordinary function statuses then propagate.

**Lifetime / время жизни.** No new ownership transfer beyond the ordinary owning-context API; callbacks/userdata are borrowed and scalar outputs are copied.

**Behavior / поведение.** Arguments/result/receiver follow ordinary function ownership. Null options preserve policy; the existing activation and GC path remains authoritative. See [execution modes](EXECUTION_MODES.md).


## fvm_debug_frame_count

Возвращает число заимствованных активаций во время живой остановки.

Declaration / декларация: [`flashvm/debug.h`](../include/flashvm/debug.h).

**Parameters / параметры.** `context`: owning VM in a breakpoint or fault debugger callback. `count`: required size_t output.

**Results / результаты.** `OK`; `NOT_FOUND` with count=0 outside a live stop; `INVALID_ARGUMENT` for null context/output.

**Lifetime / время жизни.** Count is copied; frame views remain live only until the callback returns.

**Behavior / поведение.** Up to 256 frames are exposed, innermost first. The API requires no postmortem report and therefore works with crash action NONE. Pair with frame_at and live argument/local/register queries; it does not asynchronously suspend a running thread.


## fvm_debug_frame_at

Читает живой кадр функции без построения postmortem-отчёта.

Declaration / декларация: [`flashvm/debug.h`](../include/flashvm/debug.h).

**Parameters / параметры.** `context`: owning VM inside a breakpoint or fault debugger callback. `index`: innermost-first frame. `out`: required FVM_DebugFrame view.

**Results / результаты.** `OK`; `NOT_FOUND` outside a stop or beyond the live/capped frame range; `INVALID_ARGUMENT` for null arguments.

**Lifetime / время жизни.** Function label, AVM1 and native mapping views are borrowed until callback return. They are not owned copies and cannot be freed/written.

**Behavior / поведение.** Fields expose actual engine, supplied argc, existing register count, current/suspended action offset, forced-entry success and preservation_status. Use source_position on its body/offset for caller source lines. Inspection adds no local spill and invokes no script. The native mapping is protected while active; do not destroy or mutate VM/code during the callback.

