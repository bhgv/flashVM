# Captured live roots / Захват живых корней (runtime0.155)

A bind of a remote directory or file now holds the selected node through an
unopened remote fid. Replacing or renaming its original source path does not
redirect the bound root. Its contents, directory members and operation permissions
remain live. Capturing a subtree does not enumerate or copy a remote tree.

Bind удалённой папки или файла удерживает выбранный узел через отдельный unopened
fid. Замена исходного пути не перенаправляет bind. Содержимое, состав каталога и
права операций продолжают меняться. Это захват корня, а не снимок дерева.

## Guest use / Использование из Limbo

The native host first installs an authorized Styx provider at `/remote` using the
[existing mount API](MOUNT.md). Existing namespace permissions and prepared-view
policy apply. The following calls use the same native per-node capture path:

```limbo
# Existing visible directory targets / существующие видимые каталоги.
sys->bind("/remote/source", "/work", Sys->MBEFORE);
ctl->bind("/remote/source/sub", "/selected", Vfsctl->MREPL, 0);

# Capture with a rights ceiling, then compose/mount an ordinary prepared view.
root := api->capture("/remote/source", 0, Vfsexport->Read|Vfsexport->Execute);
view := api->overlay(root, nil, "/private", 0, Vfsexport->Tree);
api->mount(view, "/public", 0, 0);

# One file can be grafted at an absent filename inside an existing directory.
ctl->bind("/remote/source/report", "/output/report", Vfsctl->MREPL, 0);
```

Here `sys`, `ctl` and `api` are loaded Sys, Vfsctl and Vfsexport modules;
`ctl->init()` precedes its use. Check each return code/root before continuing.
See the [complete interpreter/JIT guest](../../runtime/fixtures/capture-bind/main.b).
File grafts retain [single-file ordering and sibling visibility](SINGLE-FILE.md).
Directory unions retain MBEFORE/MAFTER and signed MINS/move ordering. A namespace
copy retains captured capabilities independently; mounting in the copy does not
change its parent namespace.

При bind после подмены `/remote/source` новый bind выбирает новый узел, а старый
сохраняет прежний. Потомки старого каталога разрешаются заново относительно его
fid: добавленные файлы видны, а отдельно захваченный потомок сохраняет свой узел.
Удалённый сервер определяет доступность удержанного fid после удаления объекта.

## Native APIs / API на C

| API | Contract / Контракт |
|---|---|
| `styxclientwalkfrom(base, relative)` | Independent fid relative to a pinned unopened base; dot clones it. Rejects unsafe paths, opened/poisoned bases and descendants of a file before I/O. Caller owns the result or receives NULL. |
| `styxmountproviderat(client, relative, access)` | Consumes the client on every outcome; walks/stats one selected root and returns an owned ordinary provider or NULL. Access remains a fixed host ceiling. |
| `dvm_provider_create_capture(ops, state, capture, fixed_root)` | Optional immutable native capture method. Adopts state only on success. `fixed_root=1` promises fixed dot identity; descendants remain live routes. `0` makes no fixed-dot promise. |
| `dvm_provider_capture(provider, relative)` | Borrows a pinned source, returns an owned captured root or NULL. May perform native I/O outside namespace/lifetime locks. Reuses an already fixed dot by retaining the same provider. |
| `dvm_provider_capturable(provider)` | Method availability only; capture of a particular node may refuse. |
| `dvm_provider_bindable_at(provider, relative)` | Existing stable-route promise or fixed dot; no I/O or permission grant. |

The `DVM_ProviderOps` layout and existing stable-bind flag are unchanged. Legacy
factories use `dvm_provider_create`. A capture callback returns a fresh, unshared
factory with no origin link, preserving the source's authority and policy. The
SDK supplies its origin pin and invalidation bridge before publishing the result.
Providers declaring global stable-relative-name identity retain that contract;
ordinary remote directories declare per-node capture instead.

```c
/* client is already connected and authorized; this consumes its owned ref. */
DVM_Provider *root = styxmountproviderat(client, "source/sub", FVM_ACCESS_READ|FVM_ACCESS_EXEC);
if (root) {
    int result = dvm_stack_mount(group, root, "/selected", DVM_NS_BEFORE, 0);
    /* Handle result:1 committed,0 stale revision,-1 refused. */
    dvm_provider_release(root);
}
```

`Vfsctl.Layer.capabilities` now reports `Capturebind` (2) for the optional method.
`Stablebind` (1) describes the observed stable route or already captured dot.
Captured source and existing union-target kinds are rechecked before publication;
a directory/file replacement during capture refuses the bind without changing
the target. Neither bit grants access. Actual open/read/write and live prepared-view policy
remain authoritative; unchanged stat mode cannot override a failed operation.

## Prepared views and notifications / Подготовка и уведомления

Capturing a prepared subtree anchors its selected base node and copies explicit
mapping/hiding definitions relative to the new root. Underlying directory members
are not copied. Explicit substituted sources keep their existing live routing
definitions. A retained native policy proxy translates checks back to the original
virtual policy path without reopening the original source pathname for I/O.

Each newly captured provider pins its origin until retirement. This keeps an
existing source feed and policy alive after source-owner release or unmount.
Source change classes are forwarded automatically into the captured provider's
local journal. Resource scope is cleared because source IDs are not captured-view
IDs. Generations belong to the receiving journal; notifications invalidate
assumptions, not permissions. Existing notice ordering/coalescing semantics apply.
Ordinary `/dev/sysevents` subscriptions can consume those notifications without a
Limbo forwarding loop or participation by the affected guest thread.

An independently constructed `styxmountproviderat` has its own automatic connection
health watch. It does not inherit a separate provider's configured content feed:
use SDK capture of that provider to retain its feed. No event endpoint or decoder
is guessed, and no new Styx message format is introduced.

Фоновое отслеживание остаётся native. Захваченный корень удерживает источник и
получает его invalidation-события автоматически; Limbo может читать уведомления.
Дополнительные права не появляются, схема wire-сообщений Styx не меняется.

## Lifetime and verification / Владение и проверки

Final captured-provider destruction signals an independent native fid-retirement
worker without joining it or waiting for a clunk RPC. That worker owns only its
fid/client capability and synchronization state, with no provider or guest pointer.
One such cleanup worker is allocated per new remote captured root. Existing bounded
best-effort clunk/transport failure semantics apply; no reconnect/replay is added.
Capturing/binding itself may perform ordinary native RPC, like an open operation.
There is no guest cooperation, scheduler polling, debugger pause or global stop.

The native suite has138 tests. Added original-codec TCP cases cover root/file and
nested captures, source-path replacement, live directory changes, operation-time
revocation/restoration, hidden/substituted prepared nodes, source invalidations,
signed insertion/reorder, namespace copies and preserved native siblings. Separate
tests check policy-path rebasing, exact factory destruction and retirement while
cleanup is deliberately held at the RPC boundary. ASan/UBSan runs use
`ASAN_OPTIONS=detect_leaks=0` on Linux amd64.

The dedicated Dis guest runs in interpreter and JIT, through real Sys/Vfsctl/
Vfsexport calls over a private native dynamic provider. It checks capability bits,
source replacement, refused source/union-target kind changes during capture, prepared hiding, a shared root in a child namespace, source
unmount and exact final factory/origin release. Other platforms are not claimed.
