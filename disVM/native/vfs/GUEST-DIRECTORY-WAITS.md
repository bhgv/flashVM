# Guest directory waits / Гостевые ожидания каталогов (runtime0.160)

`Sys.dirread` now registers an admitted native cancellation wait for opt-in
provider directories, prepared views and supported namespace/union cursors.
Existing `Processctl.cancelwait` and `killwait` can interrupt the native provider
operation. Delivery does not depend on a guest event loop, guest polling,
debugger pause or a global execution stop. Each guest keeps its own native thread.

A successful directory batch remains a normal array of `Sys.Dir`. A cancelled
operation with no acknowledged visible entries returns failure, not EOF. A batch
already acknowledged by the provider can still be returned. After cancellation,
the cursor is invalidated: open a new directory FD for a fresh enumeration.
Ordinary EOF remains a successful zero-entry result.

## Provider contract / Контракт провайдера

A trusted native factory opts in through both flags:

```c
dvm_provider_create_native(&ops, state, capture, fixed_root,
    DVM_PROVIDER_CANCEL_PATH | DVM_PROVIDER_CANCEL_LIST);
```

`DVM_PROVIDER_CANCEL_LIST` is2 and requires PATH1. It promises that directory
open/list and the close of every returned stream consume the current native
cancellation scope. Every successfully opened directory must advertise
`DVM_VFS_CANCEL_LIST`. This includes cleanup of temporary cursors, not just data
reads. Old constructors opt out. Callback table layouts and the guest Sys ABI
are unchanged; rebuild native SDK consumers when updating adapters.

The fd-owner Styx adapter fulfils this contract with original Tread/Tflush and
cancellable Tclunk. A refused binding setup or unacknowledged cancelled clunk
uses independent native cleanup; it never makes the caller wait on an
uncancellable cleanup RPC. No data/open replay or fid-number reuse is added.
The [control API](CONTROL-WAITS.md) and [native enumeration rules](DIRECTORY-CANCEL.md)
continue to apply.

Prepared views propagate the promise conservatively across their immutable
source mappings. Their local policy callback must make bounded native decisions;
provider I/O belongs to the source callbacks. A view whose inner provider returns
an acknowledged batch after cancellation applies its local visibility/rights
mask to that batch without issuing new source queries. This preserves checked
prefixes through nested views. If cancellation interrupts this view's own child
metadata query, only its already checked prefix is retained. Explicit mappings,
hidden names, basename validation and duplicate suppression remain enforced.

Флаг LIST провайдера означает отменяемость всей цепочки open/list/close, включая
промежуточные курсоры. Подтверждённый batch проходит локальную политику каждого
внешнего view без новых запросов после отмены. Отмена не даёт дополнительных прав
и не разрешает раскрывать скрытые mappings.

## Live namespace / Живой namespace

At open, directory eligibility checks the retained base stream, all current union
candidates and immediate child binding routes. These checks copy/pin native
capabilities and perform no provider I/O. The eligibility snapshot is discarded;
it does not freeze the tree or prevent later mount/bind/reorder/unmount.

Before each newly selected union provider, the cursor checks its LIST factory
promise. Before each projected child metadata lookup, it checks all retained PATH
candidates. An already opened capable branch remains attached to its original
node while its index follows the live mount-head order, as before.

A late capable provider is usable normally. A late legacy/host-native route
cannot silently turn an admitted cancellable call into a blocking legacy call.
It fails before invoking that route, with:

```
directory provider lacks cancellation support; reopen directory
```

The interrupted cursor is unusable. Reopening computes eligibility again. A
mixed or legacy directory keeps its existing admission path, including ordinary
union enumeration. Under an explicit native cancellation scope, a failed branch
list also terminates enumeration; a cancelled/poisoned cursor cannot masquerade
as EOF or trigger another provider. Ordinary unscoped union failure handling
remains unchanged.

Поддержка отмены проверяется повторно при выборе нового источника. Если после
open добавлен legacy bind, текущий отменяемый dirread завершается ошибкой до его
callback. После переоткрытия mixed-каталог использует прежний путь допуска.
Namespace остаётся изменяемым, mutex группы не удерживается во время I/O.

## Ownership and publication / Владение и публикация

The calling guest pins its FD, reserves its FIFO ticket and registers a native
source before leaving managed execution. Provider methods borrow that scope
through synchronous native dispatch. Signal callbacks retain only native tokens
or independent wake state, never a guest, provider or cursor pointer.

After list completion, the caller unlinks the wait and clears TLS before managed
heap reentry. Cancellation invalidates cursor continuation, while acknowledged
entries remain available for publication. The FD ticket stays admitted through
protected construction of the managed result. On publication failure the cursor
is aborted. Only then are the ticket, native batch and FD pin released. Another
reader of the same FD cannot consume the next batch during result construction.

## Verification and remaining work / Проверки и оставшаяся работа

The real native Dis guest fixture runs in interpreter and JIT with native spawn:

- Six admitted waits cover cancelwait/killwait for plain, prepared-view and union
  directories, with exact native job and FD retirement.
- Two more waits preserve an acknowledged raw batch and a prefix through nested
  prepared views; both invoke the real protected managed publication callback.
- Two late-bind cases exercise union and projected-child rejection through actual
  Sys.dirread and verify zero legacy callbacks after admission.
- Source-thread probes verify cleared TLS before managed reentry/publication and
  retention of the native FD ticket. A private negative control removing scope
  registration is rejected by the actual native directory method.

The native original-codec TCP suite adds six scoped directory-close cases:
success, pre-cancel, cancellation, reply/flush race, lost flush and timeout. Native
view tests verify hidden names and a visibility revocation during an acknowledged
read/cancel race. Native namespace tests also cover capable late binds, unsupported late binds, refreshed
eligibility after reopen and no lower-provider fallback. The complete native
suite now has248 cases and runs normally and under ASan/UBSan.

Guest transport installation still uses embedding APIs; this does not add a
Sys.mount transport builtin or authentication. Guest close itself and bind/capture
acquisition remain separate cancellation integration. Other host platforms are
not claimed by Linux amd64 acceptance.
