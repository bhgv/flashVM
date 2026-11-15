# Remove an entry through a held directory

`fvm_port_anchor_remove_at(parent, relative)` in [anchor_port.h](../ports/anchor_port.h)
removes a file, final symlink or empty directory against a borrowed live directory
anchor. Parent must outlive the complete blocking call, just as for relative
open/stat. No file cursor or parent ownership transfers. Independent operations
may use the same immutable parent concurrently; close waits for borrowers.

Success returns [FVM_OK](../include/flashvm/flashvm.h#L44). Invalid parent/path returns [FVM_INVALID_ARGUMENT](../include/flashvm/flashvm.h#L47);
OS failure returns [FVM_ERROR](../include/flashvm/flashvm.h#L45). The absent Windows relative HANDLE backend returns
[FVM_UNSUPPORTED](../include/flashvm/flashvm.h#L48) without pathname emulation. No VM callbacks, installed helper,
external library, recursion or allocation is introduced in the POSIX operation.

The relative path must be nonempty and nonabsolute, within existing UTF-8 bounds.
Final dot/dot-dot and empty final component (including trailing slash) are
refused. Intermediate symlinks and native dot-dot traversal retain the existing
host semantics; this is not containment or a permissions sandbox. POSIX uses
fstatat with AT_SYMLINK_NOFOLLOW to choose file unlink versus empty-directory
removal, then unlinkat. A final symlink, including a dangling link or link to a
directory, is unlinked itself; its target is not deleted. A nonempty directory
fails, preserving children.

The child can change between classification and removal. This is removal by
selected parent/name, not compare-and-delete by child inode. Same-kind external
replacement can be removed; a changed kind may cause refusal. The API does not
retry through another pathname or roll back a successful deletion. Exactly one
of concurrent successful removals of an unchanged name wins; other calls fail.
No durability/fsync transaction or recursive deletion is promised.

POSIX opened files remain usable after their name is removed. DisVM retains the
same namespace selection through native release/reentry and guards only scalar
result publication; it never repeats deletion merely because the result cell
is busy. [DisVM contract](../../disVM/runtime/NAMESPACE-REMOVE.md).

Linux x64 real-port tests cover held-parent rename with decoys, files, empty/full
directories, final file/directory symlinks, eight OS owners racing one removal,
invalid paths and an FD still readable after unlink. The existing independent
cursor/create/late-stdio-failure tests remain active. Interpreter/JIT examples
also keep live Module code after deleting its image. macOS/Android physical
acceptance and Windows relative backend remain open. No Flash stage closes.

[Directory anchors](DIRECTORY_ANCHORS.md), [Russian](DIRECTORY_REMOVE.ru.md).

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_ERROR` | enum constant | [include/flashvm/flashvm.h:45](../include/flashvm/flashvm.h#L45) |
| `FVM_INVALID_ARGUMENT` | enum constant | [include/flashvm/flashvm.h:47](../include/flashvm/flashvm.h#L47) |
| `FVM_OK` | enum constant | [include/flashvm/flashvm.h:44](../include/flashvm/flashvm.h#L44) |
| `fvm_port_anchor_remove_at` | function | [ports/anchor_port.h:78](../ports/anchor_port.h#L78) |
| `FVM_UNSUPPORTED` | enum constant | [include/flashvm/flashvm.h:48](../include/flashvm/flashvm.h#L48) |
<!-- source-links:end -->
