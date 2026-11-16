# Provider data cancellation / Отмена ввода-вывода провайдера (runtime0.157)

`Sys.read`, `Sys.readn` and `Sys.write` now keep a native cancellation record
through already-admitted I/O on explicitly interruptible provider handles.
`Processctl.cancelwait` and `killwait` can therefore reach an active Styx Tread/
Twrite through the FD queue, provider wrapper and prepared view, using existing
Tflush fences. The affected guest does not poll a mailbox, run a callback or pause
for inspection. Its permanent native owner finishes its ordinary I/O continuation.

Отмена теперь охватывает не только очередь допуска к FD, но и уже начавшийся
read/write провайдера, который явно поддерживает такую отмену. Для Styx запрос
доходит до native fd-owner и Tflush. Нет guest callback, кооперативного опроса,
debug pause, глобальной остановки или смены владельца Prog/REG.

## Capability and scope / Capability и область операции

`DVM_VfsHandle.native_flags` appends a native handle capability field.
`DVM_VFS_CANCEL_DATA` promises that synchronous data callbacks consume the current
native cancellation scope and can wake their blocking native operation. It is
not an access right. Native/provider callbacks still decide rwx, actual counts
and success. Only trusted native providers can advertise this capability.

Ordinary adopted host files and legacy providers have zero flags and retain their
existing cancellation boundary. Cancellation of a queued FD ticket still removes
only that ticket. Cancelling an interruptible admitted head signals its current
operation without removing the head's FIFO ownership. The head releases its FIFO
only after I/O finishes. Other guests sharing that FD keep their ordering.

Provider wrappers and prepared views preserve the data capability. Native Styx
mounts advertise it for regular files on fd-owner clients, including one-slot
clients. Legacy callback transports and directory cursors do not advertise it.
The callback-table layouts (`DVM_VfsOps`, `DVM_ProviderOps`) and guest Sys ABI are
unchanged. Native SDK consumers must rebuild for the extended handle structure.

Для обычных файлов и провайдеров без флага поведение сохраняется. Флаг относится
к data I/O, не к directory/control операциям и не к правам доступа. Обёртки view
и provider его сохраняют; перенос и close сохраняют линейное владение handle.

## Native API / API на C

| API | Contract / Контракт |
|---|---|
| `dvm_cancel_create/retain/release` | Owned native sticky source, independent of provider/guest lifetime. |
| `dvm_cancel_request` | Idempotent native signal; no guest calls or transport RPC. Caller retains a source reference. |
| `dvm_cancel_subscribe/unsubscribe` | Source-pinning subscription; sticky replay prevents lost cancellation. Replay may race delivery, so callbacks must be idempotent. Unsubscribe does not wait for admitted callbacks. |
| `dvm_cancel_enter/leave/current` | Synchronous per-native-thread LIFO scope. Scope owns a source pin; current is borrowed. New native threads do not inherit it. |
| `dvm_vfs_gate_enter` | Host-only rooted FD admission: register copied PID, source and FIFO ticket before native release; preserve the registration during I/O. |
| `dvm_vfs_wait_end` | Unlink and clear scope after I/O/FIFO leave, before managed reentry. Returns whether cancellation/shutdown was observed; preserves acknowledged effects. |

Native code can also establish a scope directly:

```c
#include "cancel.h"
#include "handle.h"

DVM_Cancel *cancel = dvm_cancel_create();
DVM_CancelScope scope = {0};
if (cancel && dvm_cancel_enter(&scope, cancel)) {
    /* A concurrent native controller holding its own source ref may request cancel. */
    DVM_IoResult result = dvm_vfs_read_chunk(&handle, bytes, capacity);
    /* Handle the actual result; never infer successful effects from cancellation intent. */
    dvm_cancel_leave(&scope);
}
dvm_cancel_release(cancel);
```

A synchronous provider may bind native cancellation to that source, as the
[Styx adapter does](mount_cancel.c). Subscription state must contain independent
native capabilities, never provider, Prog or guest pointers. Callbacks only signal
native readiness/cancellation. They must not wait for a target, perform provider/
transport I/O, enter the VM or reenter the host cancellation ledger. Subscribe
outside locks that the callback itself needs, since sticky replay is immediate.
Its release callback runs only after unsubscribe and all admitted calls finish.

The host wait record owns native storage and a copied PID. Cancellation is fenced
against registration and unlink by the existing native ledger. Every failure path
removes its ticket and scope before managed reentry can unwind. This uses existing
direct-child Processctl authority; it adds no supervisor permission bypass.

## Acknowledgements and diagnostics / Подтверждения и диагностика

Each Styx data RPC gets a fresh single-use native token. A source cancellation
request wakes the fd-owner through that token; the owner sends Tflush according
to the [existing multiplexer rules](../client/MULTIPLEX.md). A late admitted
callback holds only the old token and cannot target a later RPC.

An original Rread/Rwrite received before Rflush keeps its confirmed bytes, even
when cancellation was requested. Sent requests without an acknowledgement retain
uncertain effects; no write or device operation is replayed. Cancelled results
keep the CANCELLED cause through provider/view wrappers. Positive acknowledged
prefixes remain positive guest results. `readn` stops after that prefix when its
scope observed cancellation, even if the short original reply was successful;
it must not create another chunk with a fresh uncancelled source.

Successful chunks still issue no preflight/follow-up Tstat. The ordinary one-time
post-failure metadata diagnostic remains. If its source is already cancelled,
the diagnostic's fresh token is pre-cancelled and no extra RPC is sent. An ordinary
RPC timeout does not itself cancel the whole source: its existing failure-only
Tstat is still allowed, and a subsequent guest cancel can interrupt that Tstat.

Подтверждённые байты не теряются из-за намерения отменить операцию. Неизвестные
эффекты не повторяются. `readn` не запускает новый chunk после принятой отмены.
Успешный I/O не добавляет Tstat; выбранная ранее диагностика после ошибки сохраняется.

## Scope and verification / Границы и проверки

This integrates guest **data I/O** cancellation for opt-in handles. Guest open,
path metadata, bind/capture acquisition, directory enumeration and close are
separate follow-up integration; the [native control-RPC cancel APIs](../client/CONTROL-CANCEL.md)
remain available. This does not add a guest Sys.mount transport builtin, a wire
extension or filesystem authority based on cancellation flags.

The native suite has182 tests. Added real original-codec TCP cases exercise
read/write through ordinary mounts and prepared views: success, pre-cancel,
cancel/Rflush, reply/Rflush races, lost Rflush and timeouts. Wire counters reject
data replay and extra metadata queries; timeout retains exactly one failure probe.
Native scope tests cover sticky subscribe/request races, TLS isolation, LIFO
ownership and unsubscribe/source teardown with an artificially held in-flight
callback. Production callbacks do not wait.

The dedicated Dis guest runs interpreter/JIT cases for admitted read/write/zero
write, cancelwait, killwait, a successful short readn acknowledgement racing
cancellation, fresh subsequent operations, prepared-view/bind propagation and a
shared-FD queued tail. It verifies exact job/FD retirement and cleared scopes
before managed reentry. Existing uninterruptible native-I/O/stream/namespace tests
remain regressions. A private relink removing only the readn cancellation stop
issues a second provider chunk; the independent fixture counter rejects it.
Production objects are unchanged by this negative control. Native ASan/UBSan uses `detect_leaks=0` on Linux amd64;
these tests do not claim execution on other platforms.


Runtime0.158 adds [guest open/stat/fstat/seek cancellation](CONTROL-WAITS.md),
including source-scoped temporary-fid cleanup and stopped union fallback.
Directory enumeration and capture/bind acquisition remain separate.
