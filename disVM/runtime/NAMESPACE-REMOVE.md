# Sys.remove through a retained namespace

`Sys.remove(path)` now removes an ordinary file, final native symlink or empty
directory through the caller's namespace. It returns0 on success, -1 with a
guest error on lookup/native/admission failure. A rooted native Sys frame is
captured before conversion/allocation. Embedded NUL and oversized paths refuse
without filesystem effects. Nil String uses the existing nil-reference error
contract (the bootstrap may represent an empty literal as nil); allocated empty
paths refuse. The debugger's native-builtin fault example now uses remove(nil),
not an unavailable service stub, retaining its real caller frame before unwind.

The source captures one retained directory plus relative name under its namespace
mutex. Removal runs outside the mutex and managed admission; it never reopens
that directory's display spelling. Rebind/unmount and host parent rename cannot
redirect an already selected operation. Source-local cleanup retains the plan
through native return, cancellation and unwind. No guest FD slot or persistent
per-object statistics catalog is allocated.

Only scalar result publication uses the ordinary shared-cell guards. A busy
result cell retries publication, not deletion. Cancellation before native
admission performs no operation; cancellation after native admission may observe
an already completed deletion. No rollback or automatic replay is promised.
There is no global stop/exclusive or live REG restore/migration.

## Behavior and limits

POSIX uses [directory-relative removal](../../flashVM/doc/DIRECTORY_REMOVE.md).
A final symlink is removed itself, including a link to a directory or a dangling
link. Nonempty directories fail without recursion. Intermediate symlinks still
follow host rules. Lexical virtual normalization precedes routing as for open;
this is not an Inferno Chan component walk or security sandbox.

Root/current held-directory selections whose relative name is dot are refused;
removing a directory binding itself is not implemented by this adapter. Its
children may be removed. A file binding retains parent+basename, so removing
through its virtual name deletes that selected source file, preserving the
physical file underneath the virtual target. The binding record persists until
explicit unmount; subsequent access fails until a source appears or route changes.

An opened POSIX FD remains readable after unlink, and an already loaded Module
keeps its code/private MP. A subsequent load of the removed image fails even if
its old Module remains cached. Namespace revisions track namespace mutations,
not native child-file existence/content; remove does not advance that revision.
Current namespace debugger text is display provenance, not filesystem metadata.

Children can change between native type classification and unlinkat. This is
parent/name removal, not an inode compare-and-delete transaction. Same-kind
external replacement can be removed; kind changes can refuse. Permissions and
host umask/access policy remain native. No recursive/secure-erasure/durability
or full provider semantics are claimed. Windows relative backend returns
UNSUPPORTED; macOS/Android POSIX selection still needs physical acceptance.

## Executable checks

The [public example](../examples/namespace-remove/README.md) runs four dedicated
native owners with copied groups. It removes files, empty directories and
bound-file sources; refuses nonempty/root/dot/NUL paths; keeps an opened FD and
loaded provider valid after unlink; checks later load refusal and exactly one
winner removing an unchanged shared name. Host rename installs actual old-name
decoys. Normal runs delete only original children; omission reopens one display
path and must actually delete the decoy, leave original present, report detection
and retire normally. A probe requires exactly four shared-result removal calls
(no effect replay) and zero live directory anchors at final shutdown.

Both dispatch modes and GC polls run, with negative controls in interpreter/JIT.
Real-port tests also cover final file/directory/dangling symlinks, invalid native
paths, empty/full directories, eight OS owners and retained FD after unlink.
[Native Sys entry](vfs/remove.c), [resource boundary](vfs/resource.h),
[Russian](NAMESPACE-REMOVE.ru.md).

Validation:18 focused ordinary and19 focused TSan CTest groups passed. Final
port/directory-symlink checks, public removal, retained debugger and generation
replay passed again (four groups per build). The standalone real port passed
ASan/UBSan with leak detection disabled; no whole-runtime sanitizer/LSan claim.
Public header passed strict C89/C++98 compilation; documentation links and edited
source <=500-line bounds checked.69-workload architecture registration is not
proof of non-x64 execution; architecture CI remains separate.
