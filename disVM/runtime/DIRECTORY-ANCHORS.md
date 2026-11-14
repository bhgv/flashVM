# Retained directory resources

Namespace groups now own native directory handles for root, cwd, import base
and directory replacements. FORKNS retains a coherent copy; NEWNS retains the
selected cwd as a new root. A relative operation within cwd/import base continues
against that object after host rename or a bind covering its virtual spelling.
A more-specific descendant bind still routes by virtual components. Ordinary
spawn continues to share its parent's namespace group.

A resource plan captures names, revision and a retained directory under the group
mutex. Blocking open/stat/acquisition uses that retained handle after unlocking
and leaving managed admission. No namespace pointer is accessed during native
I/O. Commit checks the revision; stale mutations retry at most eight times.
There is no global stop/exclusive section, worker migration or saved-REG restore.

Module loading retains the same selection through cache lookup/open/parser.
Module.path is the absolute **display spelling captured at selection**, useful
for provenance; rename can make that spelling stale. It is not reopened to
perform anchored I/O. Metadata still comes from the actual opened image.
Atomic image replacement and the existing cache limitations remain documented
in [module namespace](MODULE-NAMESPACE.md).

Each in-flight plan lives at a stable address on its source owner's C stack.
Native cleanup registration releases it before unwinding that frame; copying a
live cleanup record is forbidden. Groups/bindings/plans own separate references;
unmount, parent retirement and peer rebinding cannot close another user's handle.
Last release closes outside the namespace mutex. No managed roots or continuous
per-object statistics catalog are added.

## Exact limits

Directory identity is retained. A regular-file binding retains its parent
**directory plus basename**, so atomic replacement remains visible to future
opens. It does not pin a regular file through that file's own rename/unlink.
An already opened file or directory cursor keeps its existing independent handle.

Virtual `..` is still lexical. Leaving the held cwd/import subtree returns to
virtual-root lookup; full Inferno Chan/mount-parent traversal is not implemented.
The host follows symlinks, and this is not a security sandbox. Child entries can
change concurrently between metadata and open. Paths remain limited to4096 bytes
including NUL. POSIX anchors require readable directories, not search-only access.

Linux x64 has executable tests. POSIX implementation uses openat/fstatat and
fresh fdopendir cursors; it is selected for Linux/macOS/Android, but this block
has no physical macOS/Android acceptance. Windows explicitly returns UNSUPPORTED
for the new anchor operations; its normal file API remains available. Executable
DisVM adapters outside Linux remain work, as do union/mount transports, providers,
FD/env/credential groups and dynamic C/Limbo provider loading.

## Evidence

The [public example](../examples/directory-anchors/README.md) uses four detached
native owners. The driver performs real directory rename and installs decoys at
the old names. Reads, creates, stat, independent directory cursors and module
imports must reach the original objects; explicit old-path access reaches the
decoy. Covering rebind routes absolute access elsewhere while held cwd remains.
Source-filter unmount compares full device/inode identities. Shutdown counts
successful anchor acquisitions and closes and requires zero live anchors.
A safe omission performs a real old-path read and must report the decoy value,
then retire normally. Both interpreter/JIT and GC polls are exercised.

A separate real-port test runs eight OS threads,1024 relative reads, exactly one
successful exclusive creator, independent directory cursors and a late fdopen
allocation failure. That failure must close the actual FD and preserve existing
file contents. New file creation can leave an empty file if a later allocation
fails; no unsafe pathname unlink rollback is attempted.

[Portable API](../../flashVM/doc/DIRECTORY_ANCHORS.md),
[source contracts](vfs/resource.h), [Russian text](DIRECTORY-ANCHORS.ru.md).

Validation for this checkpoint:107/107 ordinary CTest groups passed. The final
error-path cleanup adjustment was followed by six focused ordinary and seven
TSan groups; eleven earlier focused TSan groups and the complete anchor driver
also passed. The real-port test passed ASan/UBSan with leak detection disabled
because this environment cannot run LSan; this is not whole-runtime sanitizer
or leak coverage. Public header passed strict C89 and C++98 compilation. The
67-workload architecture list includes directory-anchors; ARM64/RV64 execution
is delegated to architecture CI, not claimed from this x64 run.
