# Single-file publication and leaf bind / Публикация файла и точечный bind

Runtime0.154 supports a true single-file Styx attach root and mounting/binding it
at an explicit filename inside an existing populated directory. No enclosing
export directory, host placeholder or copy of file bytes is required. A file-like
channel/device provider can use the same ordinary byte-file interfaces.

For example, mount the received file at `/work/report`. The `/work` directory,
its original identity and unrelated entries remain. Only the exact
`/work/report` stack changes. A local source file can be bound there through
ordinary Sys.bind or Vfsctl.bind using the same file-stack mutation path.

## Guest use / Использование из Limbo

```limbo
# root is a granted single-file Vfsexport->Root; /work already exists.
api->mount(root, "/work/report", Vfsctl->MBEFORE, 0);
# Ordinary local alias and explicit indexed insertion use the same leaf stacks.
sys->bind("/work/report", "/work/alias", Sys->MBEFORE);
ctl->bind("/other/report", "/work/report", Vfsctl->MINS, -1);
```

Check each result: mount/bind success is0; Vfsexport/Vfsctl can additionally
report-2 for a stale namespace revision. This snippet assumes already loaded
Sys, Vfsexport and initialized Vfsctl modules and an explicitly granted root.
Specify the destination **filename**, not just its parent directory. A file
cannot replace a directory, and a missing parent is refused. This explicit
spelling permits renaming a publication without depending on remote stat names
(which can be dot for a file root).

Capture/announce/submit/take remain the existing APIs. Direct `announce(...,0,file)`
and Prepare mode both accept one file. Preparation may choose another file than
requested. The selected file remains an ordinary live provider, with normal
operation-time permissions and independent opens. Issued grants/FDs retain their
usual lifetime after publisher close or namespace unmount.

## File stacks / Стеки файлов

| Flag/API | Effect at `/work/report` |
|---|---|
| MREPL | Replace only this leaf's stack |
| MBEFORE | Insert the source stack above existing leaf candidates |
| MAFTER | Insert below existing candidates |
| MINS | Insert at a signed slot using Vfsctl/Vfsexport's separate index |
| MAUTORWX | Existing operation/access selection across this file stack |
| Vfsctl.layers/move | Inspect/reorder this file's candidates without moving its directory |
| Sys.unmount at leaf | Remove selected/all exact layers; ordinary inherited/native route reappears |

Sys.bind now accepts explicit file unions rather than rejecting MBEFORE/MAFTER
solely because the source is a file. This is an intentional hosted extension;
standard flag values and Sys ABI are unchanged. MINS remains the separately
indexed Vfsctl/Vfsexport extension. Existing directory union behavior is retained.
MCREATE retains its existing branch policy; namespace publication of a missing
leaf does not create a native file or confer provider create authority.

A truly absent destination leaf is permitted under a visible existing directory.
The native POSIX anchored stat distinguishes ENOENT as NOT_FOUND; provider failures,
permission errors, unsafe confined symlinks and unsupported queries are not treated
as permission to create a namespace entry. Providers should report NOT_FOUND for
a genuinely absent leaf. Namespace changes still commit once at the captured
revision; allocation failures/stale revisions publish no partial stack.

## Directory visibility and lifetime / Видимость в каталоге и lifetime

Ordinary guest directory reads project exact immediate child bindings beside the
original directory entries. A bound child is emitted with its destination name
and current selected metadata; an underlying entry with that same name is
shadowed, preventing duplicate graft names. Unrelated entries pass unchanged.
Unrelated directory-union duplicate-name behavior is preserved.

The native cursor retains its original base directory and namespace independently.
Open routing reuses the kind already observed during branch selection, avoiding
an extra success-path metadata probe on ordinary byte-file opens.
Child names are captured at the first enumeration read, so a directory opened
before the bind sees grafts installed before its first read. Child metadata and
visibility are resolved at delivery. Failed allocation/list publication cannot
silently skip a consumed batch, and fully shadowed batches do not invent EOF.
Supported base rewind is retained and refreshes projected names. Normal directory
cursor enumeration rules apply: this is not a snapshot/freeze of the exported
filesystem; reopening/rewinding enumerates the current bindings, while existing
ordinary file opens keep their selected objects.

## Original Styx integration / Интеграция с original Styx

The change is owned by Inferno `experiment` commit
`46b70476accafbf46d02c2ea4e45fa5080d6d586`, then imported exactly through UPSTREAM.json.
`styxattachroot(server, pending, qid)` stages a native-authorized file/directory
Qid. Pass pending=NULL inside synchronous attach; deferred completion supplies its
live StyxAttach. Return0/-1 means staged/refused. The original Rattach codec and
allocated fid preserve that Qid's full64-bit identity and type. Legacy attach
still defaults to directory Qroot. No Styx/9P message extension is introduced.

A callback-backed file needs a distinct Qid path rather than the cached directory
Qroot; cached type conflicts and authentication Qids refuse. Flush/reset/disconnect
make retired setters/completions ineffective. Zero-element Twalk clones file fids
normally, and descendant walks on a file refuse. Admission and actual backend
operations remain responsible for authority and effective access.

A true single-file remote mount declares stable binding for dot only: its
attached root fid anchors that object for the connection's lifetime. The client
copies the immutable attach kind without an extra RPC, and the provider rejects
all descendant paths locally. Directory-root providers retain their conservative
no-stable-bind declaration until a separate retained-subroot-fid adapter exists.
There is no path-based reconnect or substitution of a new attach root. Binding
retains that provider/connection; opened fids retain their own cursors. Remote file bytes/permissions remain live.

[Native file-root server](../tests/file-root/wire.c),
[publication backend](../tests/file-root/backend.c) and
[real network mount/bind client](../tests/file-root/client.c) demonstrate both
synchronous and native-reviewed preparation paths. The standalone SDK fixture
has no VM context; its context shim refuses managed cleanup registration.

## Acceptance / Проверки

The native library suite has134 tests. Original-codec tests cover synchronous and
deferred64-bit file roots, clone/descendant behavior and retired handles. Actual
TCP tests publish one writable file, mount it into a populated native namespace,
plain-bind aliases, list new names with native siblings, exercise MAFTER/signed
MINS/move, reject missing parents/directory replacement, change live remote bytes
and unmount without changing native files or creating host placeholders. Held
file fids remain usable afterward. Both direct and prepared exports run with
ASan/UBSan (`detect_leaks=0`).

The dedicated native Dis guest case runs in interpreter and JIT using direct and
prepared file grants, Sys.bind, Vfsexport.mount and Vfsctl. It checks a directory
opened before the grafts, exact listing names, indexed reorder, automatic reads
from a readonly top file with writes to a writable lower file, surviving FDs,
restored native files and unchanged unrelated source/sibling bytes.

Один файл можно анонсировать и получить как file-root, затем смонтировать или
забиндить в `/каталог/имя`. Родительский каталог и остальные файлы сохраняются.
Правила наложения действуют на стек этого имени. Подготовка и прямая выдача
используют обычные live providers; протокол Styx не меняется и guest-потоки
не приостанавливаются.
