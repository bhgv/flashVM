# Native mounted 9P provider / Native mount-provider 9P

Runtime0.145 adds `mount.h`: `styxmountprovider(client, access)` consumes an owned
[native client](../client/README.md) on every outcome and returns an ordinary
`DVM_Provider*` or NULL. The host supplies a connected transport, native total-RPC
deadline callbacks and identity hints. Dialing, credential verification and guest
Sys builtin/FD registration are not added by this adapter.

The provider can be passed to the existing trusted native namespace mount API.
The namespace implementation retains it; independently opened handles keep the
factory/connection alive after namespace removal or caller release. The example
uses an already authorized namespace; it is not an additional nsadmin bypass.

```c
#include "mount.h"
#include "resource.h"

/* transport callbacks and namespace were selected by the native host. */
StyxClient *client = styxclientmount(&transport, 8192, user_hint, logical_tree);
DVM_Provider *provider = styxmountprovider(client, FVM_ACCESS_READ | FVM_ACCESS_EXEC);
DVM_NamespacePlan target;
int mounted = -1;
if (provider && dvm_namespace_plan(group, "/remote", &target)) {
    mounted = dvm_namespace_mount_provider(group, &target, provider, "/remote-styx");
}
/* mounted:1 committed,0 stale revision,-1 invalid/allocation failure. */
dvm_provider_release(provider);
```

The acceptance suite tests the provider/fid integration, not a guest Sys.mount
builtin. Namespace insertion itself retains its existing separate tests. The
API exposes no host path lookup through a remote file name; canonical remote
components are always resolved inside the attached root. Namespace permissions,
view policy and the selected remote server remain in charge of access.

## Operations and authority / Операции и полномочия

A fixed host access ceiling is checked before open and masks displayed rwx bits.
Metadata never grants actual server permissions. The provider walks and stats an
independent unopened fid, then Topen decides access for the actual retained fid.
Ordinary open modes are r, wo and rw/rwo. Directory open requires
host READ|EXEC and read mode. Each open has its own cursor and remote fid.
Read/write are precise one-chunk callbacks with current explicit64-bit offset;
there is no implicit Twalk/Tstat before every successful data operation. Handle
info sends Tstat for the existing fid, never reopens its pathname.

[Directory access checks](MOUNT-ACCESS.md) use a temporary read-only open
under the same host ceiling as an ordinary directory open. General file
and write access probes remain unsupported.
[По-русски](MOUNT-ACCESS.ru.md).

Directory relative-name stable-bind capability remains undeclared;
runtime0.155 provides [explicit retained-fid root capture](CAPTURE.md) for directory
and file aliases instead. Runtime0.154 declares stable bind for a true single-file
attach root, rejecting every descendant and anchoring dot to that root fid.
A changing remote tree is not a snapshot.

Wire Rerror contains a string, not portable typed errno/effect acknowledgements.
The adapter returns generic FVM_ERROR rather than pretending every failure is
NOT_FOUND or permission denial. Failed data replies without counts conservatively
poison that fid's data operations; remote prefix success remains exact. A lost
write acknowledgement is never retried. The ordinary SDK performs one diagnostic
Tstat only after failure and cannot turn a failed operation into success.

Mapped metadata currently includes basename, full Qid identity, device, length,
directory type and rwx. Numeric ownership is marked unavailable; textual owner IDs,
timestamps and additional9P mode attributes are not yet mapped into this scalar
VFS bridge. They must not be interpreted as authenticated native credentials.

## Directories / Каталоги

Directory Tread contains only packed stat records for that directory level.
Original lib9 validates/decodes every complete record. Malformed/truncated records,
embedded NUL basenames, slash, dot and dotdot entries fail the cursor; errors do
not become EOF or silently restart it. Published batches have at most the port's
batch limit. Remaining records in the same read stay buffered; subsequent metadata
changes are visible on later reads/reopens, not an atomic directory transaction.
Only an actual zero-byte successful read reports directory EOF.

Directory failure stops the current enumeration. A seek to zero from the
start discards buffered entries and begins the next read on the same fid.
[Directory rewind](../../../flashVM/doc/STYX_VFS_DIR_REWIND.md) describes
the paired runtime. Data-file seek changes only its checked local protocol
offset. SEEK_END explicitly stats the held remote fid. New directory/file paths remain
live and server-defined.

## Exclusive creation and removal / Исключительное создание и удаление

A writable directory attachment used by `--styx-mount-fd` or
`--styx-mount-tcp` accepts `Sys.create(path, Sys->OWRITE | Sys->OEXCL, 8r666)`
and `Sys.remove(path)` for descendants. Creation uses a walked parent fid
and one `Tcreate`. Removal walks and removes one exact fid with `Tremove`.
Both require a host WRITE|EXEC ceiling and permission from the peer. The
mount root and single-file attachments cannot be deleted through the mount.
Provider-backed creation requires a branch with creation enabled. Truncating
create, directory creation through `Sys.create` and `Twstat` are outside this
mounted Sys profile.

A failed guest call reports nil or -1. An interrupted request may have taken
effect if its acknowledgement was lost. The bridge preserves a confirmed
reply and does not replay an uncertain mutation. Existing open file fids
remain tied to the generations assigned by the peer.

Каталог с правом записи под `--styx-mount-fd` или `--styx-mount-tcp` принимает
`Sys.create(path, Sys->OWRITE | Sys->OEXCL, 8r666)` и `Sys.remove(path)` для
потомков. Создание посылает один `Tcreate` по fid родителя. Удаление посылает
один `Tremove` по fid выбранного узла. Требуются предел WRITE|EXEC на хосте
и разрешение сервера. Корень монтирования и отдельный подключённый файл
удалять через монтирование нельзя. Ветвь должна разрешать создание.
Создание с усечением, создание каталога через `Sys.create` и `Twstat`
не входят в этот профиль смонтированного Sys.

При ошибке гость получает nil или -1. Если прерванный запрос остался без
ответа, изменение могло произойти на сервере. Мост сохраняет подтверждённый
ответ и не повторяет запрос с неопределённым исходом. Уже открытые fid
остаются привязаны к поколениям, выданным сервером.

[Limbo and C example](../../../lib/styxrpc/examples/disvm/MUTATIONS.md) ·
[Пример Limbo и C](../../../lib/styxrpc/examples/disvm/MUTATIONS.ru.md).

## Loss notifications / Уведомления о потере соединения

Runtime0.147 replaces operation-observed mount loss with an automatic native
terminal subscription. With the fd-owner client, idle EOF/failure publishes
CONNECTION|NETWORK without another RPC; explicit shutdown publishes CONNECTION.
Normal final owner release is quiet. Each mount owns its journal subscription;
in-flight notifications safely outlive provider destruction without making
unmount wait. Existing namespace watchers route events to native thread inboxes.
[API, ownership, replay, transport limits and checks](../client/EVENTS.md).

The legacy callback transport still observes loss during an exchange. No new
heartbeat wire message, periodic Tstat, reconnect or remote-fid restoration is
introduced. Runtime0.148 adds an optional [native heartbeat policy](../client/HEARTBEAT.md)
for silent connections.

## Acceptance / Проверки

Real original C libstyx servers and an independent byte relay exercise:

- independent equal-Qid opens and two native OS threads using one session;
- 64-bit Qid/offset, live renamed directory data and operation rights unlike mode;
- retained handles after factory release, with exactly one transport close;
- exact short Rwrite, post-failure Tstat and no replay after uncertain effects;
- invalid negotiated msize, wrong response tags/counts and impossible Rwalk counts;
- iounit limits and a slowly fragmented reply bounded by one total RPC deadline;
- read-only host ceiling, actual directory-open probes and unsafe records;
- more than MAXWELEM components and immediate cleanup of a partial walk;
- prepared per-connection publication via a dedicated reviewer and native wakeup.

The complete opt-in native suite has78 tests, including ASan/UBSan on Linux amd64.
LeakSanitizer is disabled on this host. No physical ARM64/Android/Windows claim is
made. The callback client is single-flight; the fd-owner client supports multiplexing
and Tflush. Remote authentication, richer metadata and guest
Sys.mount transport wiring remain follow-up work. Directory subroot binds now use
the [runtime0.155 capture adapter](CAPTURE.md).

Это обычный native provider поверх исходных lib9-кодеков. Каждый open удерживает
свой remote fid; операции не подменяют права metadata. Частичные подтверждения
сохраняются, неизвестные effects не повторяются. Обнаруженный разрыв автоматически
даёт native-событие; fd-owner обнаруживает EOF даже в простое. Для молча пропавшей сети runtime0.148 добавляет optional heartbeat. Подготовленное дерево остаётся живым; дальнейшие режимы union/balancing и migration не ограничены.

## Concurrent fd-owner client / Concurrent fd-owner (runtime0.146)

[Native multiplexer API](../client/MULTIPLEX.md) can supply the owned client to
this same provider. Independent fids then issue simultaneous RPCs; timeout uses
Tflush, and cancelled outcomes map to the ordinary SDK CANCELLED cause. Final
provider destruction signals native owner retirement without joining it.
Explicit per-operation cancel tokens are available in the native client API;
binding them to guest FD/broker requests is a separate integration step.


Runtime0.154 adds [single-file roots and stable leaf binding](SINGLE-FILE.md).
The attached file root fid anchors exact file namespace bindings; normal remote
contents and permissions stay live. Missing explicit leaf names can be mounted
inside existing directories without hiding siblings or creating host files.

Runtime0.155 adds [captured live directory/file roots](CAPTURE.md), shared by
Sys.bind, Vfsctl and Vfsexport.capture without a Styx wire-format change.

## Exact lookup absence / Подтверждённое отсутствие имени

Metadata/open path resolution now uses the [per-call lookup result](../client/LOOKUP.md).
Only an exact acknowledged Twalk Enonexist under a host-selected Inferno profile
becomes FVM_NOT_FOUND. Default opaque errors, partial walks, stat/open failures,
denial and transport failures stay conservative. Select the vocabulary before
sharing the client or creating a mount; later changes refuse. No pathname is
reopened after a failed data operation and no unknown effect is replayed.

## Advertise a launchable file / Объявить запускаемый файл

[`styxmountproviderlaunch`](mount.h) captures a directory and one advertised
file on an already authorized Styx connection. The host supplies a trusted
native resolver for that file. Discovery checks the current walk/stat against
the held remote node identity before calling it. Capturing or binding an alias
keeps the association. A different file or directory does not acquire it.
The returned launch address keeps a separate connection reference and refuses
submission after disconnect. The resolver supplies the admission capability
and logical owner label. File names and Qids do not confer launch authority.

[`styxmountproviderlaunch`](mount.h) захватывает каталог и один объявленный файл
в уже разрешённой сессии Styx. Host задаёт native resolver для этого файла.
Перед его вызовом поиск сверяет текущий walk/stat с удерживаемым объектом.
Capture и VFS-алиас сохраняют привязку. Другой файл или каталог её не получает.
Адрес запуска удерживает соединение отдельно и отклоняет подачу после разрыва.
Полномочие очереди и логическое имя предоставляет resolver.

The launch request tests resolve an alias via the original 9P peer, delete the
caller namespace and submit the retained address into the provisioned owner
queue. This adapter consumes a host-provisioned resolver. It does not assign a
wire advertisement format or discover a queue endpoint from an unauthenticated
peer.

Проверка на оригинальном 9P получает адрес через алиас, удаляет namespace
вызывающего и подаёт запрос по удержанному адресу. Resolver задаётся host.
Получение адреса очереди по сетевому объявлению остаётся отдельным стыком.
