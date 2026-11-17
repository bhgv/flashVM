# Pre-init launch environment / Окружение перед init (runtime0.165)

This checkpoint implements provider-side **namespace preparation**, source
provenance and installation identity provisioning. It does not yet implement a
network launch service, remote module/function proxies, FD wire transfer or the
FlashVM terminal proxy. The previous local provider-image loader experiment is
kept on a separate local work branch and is not part of this release.

Реализован законченный этап подготовки окружения: два выданных живых корня,
изолированный namespace, терминал первым, просмотр происхождения и перестановка
отдельного устройства. Это ещё не сетевой удалённый запуск. Провайдерский `.dis`
не скачивается для исполнения на терминале; эксперимент такого локального
загрузчика не входит в этот commit.

## Agreed execution model / Согласованная модель

A remote published module executes at its provider. The terminal delegates a
filtered live VFS root, FD capabilities and its existing terminal interface
parameter. The provider prepares its own permitted tree, agrees to an explicit
initial union policy, constructs the instance environment and only then calls
init. Dependencies execute on the provider through that instance's namespace.
Graphics/input/output initially belong to the terminal's FlashVM. No Tk or new
DisVM graphics engine is introduced. The existing terminal interface passed to
init will receive a transport proxy; specialized per-application clients are
not required.

The eventual launch exchange must associate roots with an authenticated
transport session. Sender-supplied system IDs, nicknames and integer FD numbers
are not capabilities or authentication. Each owner chooses the tree it grants
through the existing publication preparation gap. No recursive tree snapshot
is sent: ordinary Styx walk/open/read/list continue against the live roots.

Names designate resources; the underlying VFS implementation may be a file,
channel, socket, device or another provider-defined object. Union ordering and
source inspection do not classify that backend. `/dev/camera` in the fixtures is
only an example name backed by native test bytes; no camera hardware is opened.

## Native preparation API / Native API подготовки

[`vfs/launch/prepare.h`](vfs/launch/prepare.h) defines `DVM_LaunchOptions` and
`dvm_launch_prepare`. The caller supplies two already granted providers, copied
source associations and `DVM_LAUNCH_TERMINAL_FIRST`. Unknown policy values,
noncanonical IDs, wrong source roles and different launch sessions are refused
before source I/O. There is no silent negotiation to another order.

Preparation captures both dot roots and validates directory kind outside
namespace locks. Callers may provide a native cancellation scope only for capable
routes. Pre-cancel starts no callbacks; cancellation after the first root stops
before acquiring the second. Every staged reference is released on failure.
Success returns an owned private namespace with terminal first and execution
provider second at `/`. It has no native host root anchor, so an absent resource
cannot escape into the preparation host's filesystem, even after all root
grants are unmounted. This no-native-fallback property also applies to directory
acquisition and namespace copies. The API executes no guest,
installs no foreign Prog and calls no init. The launch adapter must hand this
owned environment to the instance before init.

Both sources remain live. Existing bind/move handles the resulting stack,
including a child-local `/dev/camera` projection that leaves `/dev/keyboard`
and its parent stack unchanged. Copies retain independent topology and shared
provider contents. Open handles remain pinned to their actual source after a
reorder. This release does not yet expose FD provenance inspection.

Default launch lookup falls through only after `FVM_NOT_FOUND`; access denial,
connection failure and other errors do not mean absence. This is an explicit
property of newly prepared launch namespaces and copies, not a global change to
legacy Inferno union lookup. Explicit MAUTORWX keeps its established effective
access selection rules. Existing provider operation support, including current
create/remove and union-cwd limitations, is not expanded by preparation.

## Source inspection / Просмотр происхождения

`Vfsctl.origin(path, revision, index)` returns `(status, ref Origin)` at an already
observed nonnegative revision. It performs no provider metadata/access I/O.
Use `layer/layers` to obtain the revision and traversal positions first. Origin
contains launch-relative `role`, canonical `systemid`, `exportid`, `sessionid`
and display `nickname`. `sessionid` is the common launch session, distinct from
the provider journal epoch in Layer.session. Nickname is copied at preparation;
renaming installation metadata affects later preparations. This identifies the granted root source, not proof of a
physical device location behind further provider proxies. Roles are `Terminal` and `ExecutionProvider`.
An ordinary unannotated route returns Ready/nil; a revision race returns Stale/nil.
Bind/move and namespace copies preserve copied source association. Origin is
not a permission grant or a prediction that a later open must choose that layer.
The native text ABI is extended by part numbers2..6; existing0/1 remain unchanged.
Native resource/binding layouts grow, so native SDK consumers must rebuild.

```limbo
(status, revision, layers) := ctl->layers("/dev/camera");
source: ref Vfsctl->Origin;
(status, source) = ctl->origin("/dev/camera", revision, 0);
# After inspecting source and the stack, move the top camera to the bottom:
result := ctl->move("/dev/camera", revision, (hd layers).id, -1);
```

В Origin показана принадлежность ресурса этому запуску, а не «доверие по имени».
Программа может переставить камеры обычным move; открытый ранее FD остаётся
с прежней камерой. Чужой namespace или global scheduler для этого не меняется.

## Installation identity / Identity установки

[`tools/disvm-system-id.py`](../../tools/disvm-system-id.py) provisions32 random
bytes encoded as canonical unpadded base64url. It reads/writes only an explicitly
selected file. Concurrent first creation atomically publishes one complete JSON;
later calls keep systemid. Nickname changes atomically replace metadata while
preserving identity. Invalid existing data is refused rather than silently
regenerated. No MAC/CPU/root-path fingerprint is required. Identity is public
metadata, not an authentication secret.

```sh
python3 tools/disvm-system-id.py --file /path/to/installation/system.json --nickname 'Лисичка'
```

A copied identity file preserves the same installation identity; an independent
clone must provision a fresh file. Export and session IDs have the same canonical
representation but belong to the grant/transport lifecycle; the future launch
adapter must allocate/associate them. The SDK validates shape and association,
not uniqueness or credentials.

## Verification and next boundary / Проверки и следующий этап

Native tests cover policy/identity/session/type/cancel refusal, live roots,
absence-only fallback, no host fallback, independent namespaces/copies, per-device
move, bind provenance and an old open handle after reorder. Private omissions of
origin propagation, strict lookup and no-native-fallback guards are detected
by actual SDK tests.
The identity tool is tested with16 concurrent creators, repeated reads, nickname
update, independent installations and malformed existing data.

The actual interpreter/JIT fixture loads bootstrap interfaces, installs an SDK
prepared namespace through a private bootstrap hook, then calls appinit. Ordinary
Sys/Vfsctl calls inspect both camera origins, reorder only the camera, reject a
stale provenance query, retain an old FD and bind a camera alias. This is a local
pre-init environment test, not evidence of a network launch. Source/namespace
ownership drains through the existing native lifecycle.

Next: authenticated prepare/accept exchange and launch tickets, reverse terminal
VFS transport, FD and terminal-interface proxies, and provider-side module launch
with dependency handling. These are still required for end-to-end remote execution.

## Native Styx root adapter / Native Styx-корень

The [native SDK adapter](../native/launch/README.md) now captures an already
attached terminal Styx client and feeds its live VFS root into this namespace
builder. It accepts ordinary sessions by default and explicitly configured bidir
channels identically. It does not start guest code or create a network launch
service. See its documented conservative missing-name fallback limitation.
