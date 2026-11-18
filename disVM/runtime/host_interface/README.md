# Unibus launch binding / Привязка Unibus к запуску

[`Unibus`](../../module/unibus.m) is the shared application host interface,
not an IDE execution mode. The runtime registers `$Unibus` before loading guest
modules. Each call resolves the active process's native binding; native module
handles do not carry another process's authority across a spawn.

## Native setup / Native-настройка

[`unibusbinding`](../../include/disvm/unibus_binding.h) copies `UnibusOps` and
adopts its native state on success. Failure leaves state with the caller. The
callback code and native state must remain valid through the last reference;
an owning `destroy` callback can hold the provider/library/transport lease.

After normal namespace preparation/take, the trusted native host uses
`dvm_launch_handoff_unibus(handoff, binding)` to retain it once in the exclusive
handoff. The caller retains its original reference. The adoption transaction
claims a unique opaque native owner identity and moves the handoff reference
into the unpublished process. PID assignment happens later; a numeric PID is
not used as the unique pre-init authority or serialized as a native pointer.

A refused adoption retains the source namespace/binding in the handoff.
An already claimed binding cannot be installed into another job. After successful
adoption, later startup failures release the process's native owners rather than
pretending to restore an already consumed handoff. Unpublished sysevents inboxes
are retired/unsubscribed on rollback before their owner reference is dropped.

Runtime принимает binding до первой инструкции приложения. Отказ до adoption
сохраняет source handoff; успешный adoption переносит ref в process. Native owner
identity назначается до PID, не передаётся по сети и не открывает права по строке
имени модуля. Незавершённые sysevents-подписки при откате освобождаются.

## Policies / Политики

| Flag | Effect |
| --- | --- |
| `0` | Binding stays with this process; spawned children do not receive it. |
| `UNIBUS_INHERIT` | Children receive retained service authority. By default it survives initial-thread exit until the last reference or explicit revocation. |
| `UNIBUS_OWNER_PORTS` | Optional restriction: ordinary `attach/detach` belong only to the original live owner. Reuse of a retired native address cannot recover that permission. |
| `UNIBUS_CLOSE_WITH_OWNER` | Optional restriction: initial owner exit revokes inherited service authority. |

The flags combine. They are native launch configuration, not new wire tags,
new bytecode instructions or a claim that every runtime service already exists.
Ordinary VFS/FD access remains governed by its existing namespace/descriptors.

`unibusretire` explicitly revokes once. Process retirement calls
`unibusownerretired`, which follows the chosen lifetime policy. Last-reference
release performs cancellation, if not already revoked, then final native disposal.
Callbacks run outside the portable per-binding mutex. Already admitted calls may
overlap cancellation; their process references keep native state alive until they
finish. Inherited handlers must synchronize their own native shared state.

По умолчанию наследованный сервис живёт до последнего ref, как унаследованный FD.
Ограничения owner-only и закрытие вместе с исходным потоком выбираются явно.
Выход одного ребёнка не отзывает права остальных; завершение исходного `init`
не обязано отключать оставшихся потомков.

## Calls and compatibility / Вызовы

`invoke(state, pid, method, frame)` executes synchronously on the calling guest's
active native owner, using the generated native-cell `.m` ABI. `frame` is borrowed
only through that call. It must not be retained, sent over a channel or serialized;
transport adapters exchange copied values/S-records. Any callback doing native
blocking work must follow the runtime's ordinary root-capture/native-wait rules.

Selectors are native dispatch metadata, not new network opcodes. Return 1 after
handling the operation, 0 for unavailable before effects; the runtime never
retries the callback or falls back to a different job's UI after refusal. Denied
owner-only attach returns -1; other unavailable/revoked operations raise on the
calling owner. An isolated handoff or its non-inherited child cannot fall back to
the host's unrelated legacy local renderer.

The optional legacy desktop backend implements the same interface through local
engine adapters. The shared runtime has no requirement for a FlashVM context:
console modules with nil display/session still use normal FD 0/1/2 and files.

## Acceptance / Проверки

`test_unibus_binding.py` runs real Dis interpreter/JIT code against the unchanged
function signatures: pre-init data, allowed/refused inheritance, owner-port
admission, unavailable methods, explicit revocation, failed adoption and exact
native cleanup. It also loads a typed console application with nil terminal,
feeds stdin, checks stdout/stderr separately and reads/writes a real ordinary file.
The native unit fixture checks default shared-service lifetime, optional policies
and retired-identity reuse. These tests do not implement remote renderer RPCs.
