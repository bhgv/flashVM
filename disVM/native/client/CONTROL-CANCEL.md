# Cancellable control RPCs / Отмена служебных RPC (runtime0.156)

Native fd-owner clients can now cancel `walk`, `open` and `stat` with the same
tag/Tflush fences as data RPCs. A normal reply acknowledged before Rflush wins
over cancellation intent. No guest cooperation, scheduler poll, debugger pause
or global stop participates in cancellation or cleanup.

Native-клиент с fd-owner теперь умеет отменять `walk`, `open` и `stat`, а не только
чтение/запись. Ответ операции, полученный до Rflush, сохраняется. Отмена будит
native owner через существующий канал, без участия или приостановки гостя.

## API and outcomes / API и результаты

| API | Ownership and result / Владение и результат |
|---|---|
| `styxclientwalkcancel(client, relative, cancel, result)` | Borrowed pinned client/path/token; owned independent fid or NULL. One logical walk may contain several MAXWELEM-sized RPCs. |
| `styxclientwalkfromcancel(base, relative, cancel, result)` | Same, relative to a borrowed pinned unopened fid. Dot clones; unsafe paths, file descendants and opened/poisoned bases refuse before wire effects. |
| `styxclientopencancel(fid, mode, cancel, result)` | Borrowed pinned, caller-serialized fid; return1/0. Modes0/1/2 retain the existing non-create/non-truncate contract and Ropen iounit. |
| `styxclientstatcancel(fid, output, cancel, result)` | Borrowed pinned, caller-serialized fid; return1/0. Metadata output changes only on a complete valid acknowledged Rstat. |

All four require a `styxclientmountfd` client and a non-NULL `StyxTransfer *result`.
For these control calls `count` is always0, `confirmed` describes that zero byte
count, and `success` describes the complete operation. `cancelled` means the
native cancel/deadline path completed without an original acknowledgement.
`uncertain` marks a sent operation without a trusted original reply. The
`confirmed` zero count must not be interpreted as successful control execution.
Invalid inputs, legacy callback clients and missing result storage refuse locally.
Passing NULL for `cancel` retains the ordinary request deadline without a token.

The existing ordinary `walk`, `walkfrom`, `open` and `stat` signatures remain.
This change adds native client APIs; it does not connect guest kill/FD requests
or Sys.mount to these tokens automatically.

### One token per logical operation / Один токен на логическую операцию

A non-NULL `StyxCancel` remains single-use. A long walk reserves its outer token
once and uses private single-flight child tokens for successive Twalk fragments.
A cancellation request is forwarded atomically to the current fragment and is
sticky across the gap before the next fragment. The next fragment cannot escape
an earlier cancellation. Once the walk finishes, late requests cannot target a
later open/stat/data call. Those calls need their own tokens.

The admitted scope/flight retains the token independently. The native control
owner may drop its own reference after admission; it must retain a reference
while issuing `styxcancelrequest`. Freeing a token alone does not cancel work.

Длинный путь разбивается по MAXWELEM, но для вызывающего это одна операция и один
внешний токен. Внутренние токены не переиспользуются. Запрос отмены между двумя
фрагментами не теряется; после завершения walk старый токен не может отменить
следующую операцию. Следующему open/stat нужен новый токен.

```c
#include "client.h"

/* Native I/O worker; client and cancel are retained through admission/completion.
 * A concurrent native controller may call styxcancelrequest(cancel). */
StyxTransfer result;
StyxFid *node = styxclientwalkcancel(client, "projects/run/input", cancel, &result);
if (node) {
    /* The complete walk was acknowledged. This token is now spent. */
    StyxCancel *metadata_cancel = styxcancelcreate();
    StyxMeta metadata;
    if (metadata_cancel) {
        int found = styxclientstatcancel(node, &metadata, metadata_cancel, &result);
        /* Handle found/result; metadata is untouched if found is zero. */
        styxcancelfree(metadata_cancel);
    }
    styxclientclunk(node); /* Ordinary bounded native cleanup, not a guest callback. */
}
styxcancelfree(cancel);
```

## Unknown open state / Неизвестный исход open

If a sent Topen has no original acknowledgement, the server may already have
opened the fid. Rflush suppresses its old reply; it does not undo that open or
device effects. The client therefore marks this fid poisoned and refuses later
open/data calls locally. It still belongs to the caller, which can query metadata
or clunk it. No second Topen is sent as an automatic retry.

An acknowledged Rerror or cancellation before any request bytes were sent leaves
the fid unopened and reusable. An acknowledged Ropen remains successful even if
a later required Rflush is lost; that loss still terminates the connection under
the existing multiplexer rules. A cancelled Tstat leaves its output untouched
and does not poison its fid. Other independent fids remain usable after a healthy
acknowledged flush, subject to ordinary RPC quota.

Без подтверждения отправленного Topen нельзя считать fid закрытым и повторять
open: сервер мог уже открыть файл или устройство. Такой fid блокирует повторный
open и данные. Подтверждённый отказ и отмена до отправки его не блокируют.
Отмена Tstat не повреждает fid и не публикует недостоверные атрибуты.

## Abandoned walks / Незавершённые walk

A failed or partial cancellable walk returns no fid to the caller. If any fragment
may have reached the server, a detached native cleanup worker owns a client pin
and the monotonically allocated abandoned fid number. It attempts one ordinary
bounded Tclunk. The cancelled caller does not wait for that RPC; other native
requests can continue while its reply is held. No provider, Prog or guest pointer
is retained by this worker.

Cleanup uses normal RPC capacity and existing best-effort clunk semantics. Under
quota/transport failure a remote fid may survive until connection teardown; no
fid number is reused, no unknown operation is replayed, and no reconnect occurs.
If allocating or starting cleanup itself fails, the connection is failed and its
native owner is signalled without a caller RPC wait. This also retires other
pending work under existing terminal-failure rules. Ordinary non-cancellable walks
retain their existing synchronous cleanup behavior.

## Verification / Проверки

The complete native suite has167 tests. The29 added cases use the original C
Styx server and original codecs over real TCP, with external reply-order gates.
They cover full18-component walks, root-relative walks, first/second-fragment
cancellation, pre-cancel, timeout, partial Rwalk, original-response/Rflush races,
lost Rflush, one-slot clients, safe acknowledged-open refusal, unchanged stat
output, token reuse refusal and an independently progressing fid while the
abandoned walk's actual Tclunk reply is withheld.

Private controls execute the production walker with second-fragment allocation
refusal or cancellation exactly between fragments. Cleanup allocation/thread/
reference refusal probes check ownership rollback and terminal observation.
A private relink with only the poisoned-open guard removed sends a second Topen;
the independent wire counter rejects that replay even when the server returns an
error. The production objects are unchanged by this negative control.
The native suites run normally and with ASan/UBSan (`detect_leaks=0`) on Linux
amd64. These results do not claim guest cancellation wiring or other platforms.
