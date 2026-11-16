# Capture and bind cancellation / Отмена capture и bind (runtime0.161)

Native operation scopes now cover guest `Vfsexport.capture`, root `info`, eligible
capability `mount`, standard `Sys.bind` and indexed `Vfsctl.bind`. Existing
Processctl cancelwait/killwait interrupts an admitted opt-in provider operation.
No affected guest event loop, cooperative scheduling, debugger pause or global
stop participates. The interpreter/JIT keeps each guest's dedicated native owner.

## Native promise / Обещание native-провайдера

`DVM_PROVIDER_CANCEL_CAPTURE` (4) is a trusted native C factory promise. It
requires `DVM_PROVIDER_CANCEL_PATH` (1). Its capture callback consumes the current
native source scope and returns a fresh rooted provider supporting PATH queries,
including temporary cleanup. It neither grants permissions nor changes 9P/Styx,
Sys.bind flags, Vfsctl mount flags or the Limbo module ABI.

```c
dvm_provider_create_native(&ops, state, capture, fixed_root,
    DVM_PROVIDER_CANCEL_PATH | DVM_PROVIDER_CANCEL_CAPTURE);
```

The fd-owner Styx adapter advertises PATH, LIST and CAPTURE (7). Original Twalk,
Tstat and Tclunk operations use fresh tokens under the same logical source scope.
The existing C client dispatches Tflush. No graphical dependency or imported
Inferno source change is introduced. Legacy clients/providers do not opt in.

Prepared views propagate immutable source promises conservatively. Stable bind
aliases and an already captured dot need no fresh capture callback. Where a
callback is necessary under an explicit scope, a missing CAPTURE promise refuses
before invoking it. A captured root without PATH is rejected and released before
its metadata callback can run. Root/origin journal references retain their
existing independent lifetime rules.

Флаг CAPTURE — обещание native-кода об отменяемости callback, а не право доступа.
Он требует PATH. При отсутствии обещания callback не вызывается под активным
scope; неподходящий захваченный root освобождается до запроса его metadata.
Стабильные aliases могут обходиться удержанием capability без нового callback.

## Guest admission / Гостевой допуск

Capture registers a native source before release from managed execution when all
retained candidates promise the required operations. This conservative check can
keep a mixed candidate stack on the legacy path even for a selected index. Root
info needs PATH. Bind/mount use one registered operation across source/target
metadata, acquisition, validation and commit when both retained selections are
eligible. Native-host routes and mixed/legacy providers retain their existing
admission boundary; the adapter does not claim to interrupt their callbacks.

Capability mount resolves its target before release, then invokes the same
transaction implementation as indexed bind. Paths and arguments are copied under
managed admission. Signal callbacks own native tokens or independent wake state,
never a provider, guest, namespace or captured-root pointer. Every source scope
is unlinked and cleared before managed reentry or result publication.

Если source или target не поддерживает нужную отмену, сохраняется прежний путь
допуска. Это особенно важно для native-host и смешанных стеков. Права и видимость
по-прежнему проверяют реальные операции провайдера. Новый флаг их не заменяет.

## Transaction boundary / Граница transaction

Preparation checks the source before each additional candidate callback and
before acquiring another root. Failed/cancelled staging releases the complete
detached chain, including captured factories, native anchors and origin pins.
Absent file-target validation checks newly resolved parent routes before querying
them, so an explicit scope cannot enter a legacy parent callback silently.

The final cancellation check occurs under the namespace mutex, before changing
binding identities, mount heads or the namespace revision. It is the commit
admission point. A cancellation observed there refuses the entire staged change;
the previous target and namespace revision remain intact. Provider I/O and native
factory destruction stay outside that mutex. Cancellation after commit admission
can lose the race: a successful commit remains successful and is never rolled
back or replayed.

`Vfsctl.bind` and capability mount map a cancelled noncommitted result to failure,
not Stale. Sys.bind may retry a genuine stale revision within its existing bound;
a cancelled attempt terminates that retry loop. A stale attempt committed nothing.
The legacy Sys.bind path also reuses its first source metadata result instead of
issuing a duplicate query after refusal or acknowledgement.

Guest capture checks cancellation after metadata/acquisition before assembling
the root view. Cancellation at that boundary discards the staged capability. A
fully completed capability can still win a later completion race. Low-level SDK
capture may return a fully acknowledged root even after a metadata/flush race;
its caller owns that root and decides whether to publish it. Subsequent bind
still performs its own transaction check. Acknowledged root-info tuples survive
cancellation without repeating the query.

Последняя проверка отмены стоит под mutex namespace до изменения bindings и
revision. При отказе старая привязка остаётся целой. Если commit уже допущен,
поздняя отмена не превращает его в ошибку: подтверждённый результат сохраняется.
Отмена подготовки освобождает весь временный стек; частичный bind не публикуется.

## Verification / Проверки

The real dedicated-thread Dis fixture runs in interpreter and JIT. Seventeen native
waits cover cancelwait/killwait for capture, standard/indexed bind, capability
mount and root info; final acknowledged type validation before commit; discarded
staged capture; acknowledged root info; and successful standard/indexed/mount
commits followed by cancellation; and a cancelled legacy-path metadata lookup
that must not be queried a second time. Independent reads verify the actual destination
identity after every operation. Native job, FD, factory and origin lifetimes must
drain exactly. A private control removing source registration is detected by the
actual native method. The guest fixture also runs ten consecutive repetitions.

Twelve real original-codec TCP tests exercise capture Twalk and Tstat success,
pre-cancel, cancellation, reply/flush races, lost flush and timeout. The relay
independently counts requests and rejects another metadata query after a cancelled
walk. A native transaction fixture tests eight commit/cancellation/unsupported
callback phases and exact namespace revision/root retirement. A private relink
removing the final commit guard makes it detect a cancelled namespace publication.
Production objects are unchanged by both negative controls.

The complete native suite has261 tests and runs normally and under ASan/UBSan
(`detect_leaks=0`). Linux amd64 acceptance also includes host-unit and guest VFS,
namespace, thread and close-backlog regressions in dynamic/static runtime builds.
This is embedding-provider integration, not a new Sys.mount transport builtin or
a network authentication implementation. [Detached close shutdown](CLOSE-CANCEL.md) now supports eligible native callbacks
in runtime0.162. Sys FD reference release remains asynchronous; no guest close syscall
is introduced. Other unregistered VFS mutation entry points remain separate work.
