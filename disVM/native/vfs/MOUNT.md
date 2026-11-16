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
Open modes are r, wo, rw/rwo without creation/truncation. Directory open requires
host READ|EXEC and read mode. Each open has its own cursor and remote fid.
Read/write are precise one-chunk callbacks with current explicit64-bit offset;
there is no implicit Twalk/Tstat before every successful data operation. Handle
info sends Tstat for the existing fid, never reopens its pathname.

Standard9P2000 has no general side-effect-free effective-access RPC. Consequently
this factory's `access` callback is unsupported; it does not substitute stat mode
or a probe Topen with possible device effects for an access decision. Direct
provider opens work through actual operations. MAUTORWX preselection and a
re-export bridge requiring effective EXEC cannot yet use this factory as an
access oracle. Native relative-name stable-bind capability is also undeclared;
ordinary mount works, but identity-preserving alias bind needs a future retained
remote-fid subroot adapter. A changing remote tree is not a snapshot.

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

Directory abort permanently invalidates that opened cursor. Rewind uses a fresh
provider open; data-file seek changes only its checked local protocol offset.
SEEK_END explicitly stats the held remote fid. New directory/file paths remain
live and server-defined. This first provider has no create/remove/wstat support.

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
- read-only host ceiling, unsafe directory records and permanent cursor abort;
- more than MAXWELEM components and immediate cleanup of a partial walk;
- prepared per-connection publication via a dedicated reviewer and native wakeup.

The complete opt-in native suite has78 tests, including ASan/UBSan on Linux amd64.
LeakSanitizer is disabled on this host. No physical ARM64/Android/Windows claim is
made. The callback client is single-flight; the fd-owner client supports multiplexing
and Tflush. Remote authentication, stable subroot binds, richer metadata and guest
builtin wiring remain follow-up work.

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
