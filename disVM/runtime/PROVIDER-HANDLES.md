# Native provider handles / Нативные provider handles

Copyright (c) 2026 bhgv

Runtime 0.123 extends an internal common owning value for opened resources.
[Handle/operations](vfs/handle.h) contain an immutable trusted callback table and
native state. [Native adapters](vfs/handle.c) implement file and directory providers.
These types do not cross the guest ABI, carry no managed roots and are not a
registration API for third-party DLLs or Limbo callbacks.

Runtime 0.123 расширяет общий внутренний owning value для открытого ресурса:
immutable callback table и native state. Реализованы file/directory adapters.
Handle не пересекает guest ABI, не содержит managed roots и не регистрирует
сторонние DLL/Limbo callbacks. Это первый подключённый слой provider dispatch,
не завершённая единая VFS.

## Connected path / Подключённый путь

[Descriptor slots](file_slots.c) now own a handle instead of separate file and
directory pointers. Existing open publication adopts one native result without
allocation. Read/write (including formatted output), seek, fstat and dirread
borrow the handle before native release and dispatch through the common table.
Unsupported operations return UNSUPPORTED without modifying output storage;
invalid/empty handles return INVALID_ARGUMENT. Actual provider errors retain
that provider's existing partial-I/O semantics.

File slots владеют handle вместо двух альтернативных native pointers. Open
передаёт один результат без allocation. Read/write, formatted output, seek,
fstat и dirread используют общую таблицу. Отсутствующая операция возвращает
UNSUPPORTED без изменения outputs; empty/invalid handle — INVALID_ARGUMENT.
Реальные I/O errors сохраняют прежнюю семантику provider, включая partial writes.

[Close worker](close_worker.c) moves the handle into a preallocated close job.
The source is cleared before queue publication; the worker clears its owning
value before invoking consuming close. Close does not allocate, enter managed
execution or run guest callbacks. Empty/repeated close is harmless. Failed moves
leave source and destination untouched. FIFO/gates, native wait cancellation,
GC admission and rooted FD lifetime remain the existing mechanisms.

Close worker переносит handle в заранее выделенный job, очищает source до
публикации и очищает owning value до callback. Нет allocation/managed entry или
вызова guest callback при close. Повторный close пустого handle безопасен.
Refused move не меняет обе стороны. FIFO/gates, отмена waits, GC admission и
rooted FD lifetime сохраняются; нового global stop/exclusive нет.

## Lifetime and limits / Время жизни и границы

A live owning handle must not be copied; use move after all borrowers retire.
A borrowed handle grants no ownership or concurrency admission. Each callback
runs under the existing source owner's lifetime pin and operation gate; final
close runs only after borrowers retire. Static callback tables must outlive all
handles. State may contain native metadata only; no VM pointers/callbacks.

Живой owning handle нельзя копировать; move допустим после ухода заёмщиков.
Borrow не предоставляет нового владения или operation admission. Таблица живёт
дольше handles; state содержит только native metadata, без VM pointers.

Stdio and loader-private reads/cache identity retain specialized adapters.
Ordinary FD operations no longer require native file/directory identity. Trusted
host code can move a valid native provider into a reserved slot using
[dvm_file_slots_publish_resource](file_slots.h). Guest code cannot register one.
A list provider must supply abort_list for failed managed publication; invalid
operation tables are refused before consuming ownership. Zero-byte writes use
native admission too, because even those provider callbacks may block. Sockets/channels/devices, provider mounts/walk, credentials/permission
policy and dynamic library lifetime remain future work. Existing host rwx and
namespace behavior are unchanged; NEWNS is not a security sandbox.

Stdio и private loader/cache сохраняют специальные adapters. Обычный FD-путь
не требует native file/directory identity. Trusted host code может передать
provider в reserved slot; guest registration отсутствует. List provider обязан
иметь abort_list для failed managed publication. Invalid tables не передают
владение. Даже zero-byte write выполняется через native admission.
Socket/channel/device providers, mount/walk, guest credentials и dynamic loader
ещё не реализованы. Host rwx/namespace не меняются; NEWNS не является sandbox.

## Checks / Проверки

[test_handle.c](vfs/test_handle.c) checks provider-independent read, missing stat/
seek with unchanged outputs, malformed/refused moves, cleared source and exactly
one close. Existing native-wide workloads exercise actual guest I/O, concurrent
FD lifetime, metadata/directory batches, cancellation, close backlog, module
retirement and namespace permissions. Architecture/TSan results apply only when
that revision's CI finishes; preceding CI does not certify this implementation.


Runtime 0.123 acceptance adds [test_provider_fd.py](test_provider_fd.py), whose
private relink replaces only named fixture file publications with memory read,
write and list providers. Eight real residents exercise ordinary Sys APIs without
native file/directory state: read/readn, write (including zero bytes), seek,
fstat, fprint, directory batches/EOF and unsupported-operation refusal. Native
host fixtures remain HOST!, while provider bytes follow ABCDE → XBCZ!. All 24
provider states must close exactly once after real native worker drain. This is
an executable provider integration test, not a user-visible mount service.

Тест добавлен в обязательный wide как provider-fd (всего 71 workload).
Восемь реальных residents работают с 24 read/write/list provider states через
обычные Sys APIs. Host fixtures остаются HOST!, provider bytes меняются отдельно.
Проверяются zero-byte dispatch, metadata, directory EOF и полный close drain.
Подмена включается исключительно private relink теста, без нового CLI bypass,
скрытого /dev lookup либо guest-доступа к native callbacks.

Runtime 0.128 appends optional precise read_chunk/write_chunk callbacks and
[chunk result diagnostics](CHUNK-IO.md). Ordinary Sys uses this path; legacy
callbacks remain supported, with unknown partial counts on legacy failure.
Runtime 0.128: optional precise callbacks, failure-only info и last native outcome;
legacy failure не выдумывает подтверждённый count. Это не stable DLL ABI.
