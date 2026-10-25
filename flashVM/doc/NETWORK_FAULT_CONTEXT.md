# Контекст сетевых исключений v0.79 / Network fault context

FVMC v1 error status допускает optional FFME v1 record длины 1220 bytes. Он
передаёт recognized native resource cause kind, resource ID, origin thread,
flags, file path, operation, message и resource authority address. Header status
остаётся источником code; OK/NOT_FOUND/OOM replies не изменяются. Если optional
allocation не удалось, сервер всё равно возвращает original error code без details.

Native resource errors получают файл failed file channel из его native handle,
если carrier path ещё не установлен. Кодек использует explicit big-endian Uint32
и fixed terminated arrays, не memcpy host structure. IDs/origin > Uint32 исключают
optional record, сохраняя status-only reply. Все данные копируются до удаления
scoped adapter context. Flags bit 8 [FVM_EXCEPTION_REMOTE_CONTEXT](../include/flashvm/exception.h#L38) обозначает,
что resource/origin/path относятся к удалённой стороне, а не local proxy handle.
Existing [FVM_ExceptionInfo](../include/flashvm/exception.h#L43) layout не меняется.

Клиент проверяет полный размер/magic/version/flags, termination каждого string и
known resource kind до изменения metadata. Некорректный/неизвестный optional record
игнорируется целиком: primary status и local fallback сохраняются. Valid details
не заменяют caller thread/source/function/line/offset. Carrier path имеет priority;
в остальных случаях переносится failed resource path. Operation/message уточняют
вложенную причину. Если cause указывает другую authority через relay, address
указывает этот resource endpoint; transport error intercept продолжает записывать
URI текущего RPC. При отсутствии cause address сохраняется request URI.

Source mapping по-прежнему предоставляется embedding/AVM1 map API; регрессионный
тест задаёт caller line вручную. Details — диагностические opaque IDs, не новые
live capability handles. Legacy brokers без FFME сохраняют старый fallback;
старые клиенты уже освобождают ignored error bodies. Negotiation/TLS/leases и
полный distributed deadlock/source graph не добавлены этим изменением.

Проверки: channel_nested_fault_network — closed mutex, queue и file channel внутри
messages, inspect/receive, native ID/origin/kind/path, caller source и working outer
queue; fault_wire — literal big-endian IDs, caller location, carrier/relay address,
short/long/magic/version/flag/unterminated/unknown/null records и unchanged output.
Оба fixed-width и header-only fallbacks сохраняют прежний код причины.

## English

Recognized FVMC v1 resource errors may carry a 1220-byte optional FFME v1 record.
It transports kind/resource/origin/flags/path/operation/message/authority address
with explicit big-endian Uint32 fields and fixed terminated byte arrays. Status
remains the primary code; OK/NOT_FOUND/OOM are unchanged. Details are best-effort
on allocation failure and omitted if IDs do not fit Uint32. Metadata is copied
before scoped adapter destruction. [FVM_EXCEPTION_REMOTE_CONTEXT](../include/flashvm/exception.h#L38) marks remote
resource identifiers without changing the existing exception-info ABI.

Complete record validation precedes all mutation. Malformed/unknown details are
ignored atomically, retaining primary status and local fallback. Caller thread
and source mapping remain local; an existing carrier path takes priority. Valid
resource details refine the operation/message and may identify another authority
behind a relay; RPC error interception still records the current transport URI.
No cause address preserves the request endpoint. IDs are diagnostic, not live
capability handles. Legacy status-only peers remain supported.

Regressions cover closed nested mutex/queue/file causes, caller locations, IDs/
origin/path, packet semantics and continued delivery. Portable codec cases cover
endianness, complete/malformed/null records, carrier/relay fields and atomic output.
Manual source locations are not automatic reconstruction. Full distributed
ownership/deadlock/source-map acceptance remains open.

Проверки v0.79 / verification: full headless 263/263; targeted fault codec/
broker/channel/resource/mutex/GC/serialization checks 74/74 in AGG, Cairo,
portable and ASan/UBSan. Portable codec runs without networking; live TCP
excluded with OS_PORTS off. Heap/global ASan and UBSan active, LSAN/stack
ASan disabled. Linux only; full distributed debugger acceptance remains open.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_EXCEPTION_REMOTE_CONTEXT` | macro / define | [include/flashvm/exception.h:38](../include/flashvm/exception.h#L38) |
| `FVM_ExceptionInfo` | struct | [include/flashvm/exception.h:43](../include/flashvm/exception.h#L43) |
<!-- source-links:end -->
