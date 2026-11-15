# Portable directory anchors

The C89 API in [anchor_port.h](../ports/anchor_port.h) exposes no OS types.
POSIX implementation retains a directory descriptor; concurrent relative
operations never change its cursor or reopen its display pathname. This is a
portable boundary, not a filesystem sandbox or a complete VFS provider ABI.

| Function | Arguments, result and ownership |
| --- | --- |
| [`fvm_port_anchor_open`](../ports/anchor_port.h#L24) | Borrow nonempty native path and output address. Open a readable directory; success transfers owned anchor, failure clears output. |
| [`fvm_port_anchor_open_at`](../ports/anchor_port.h#L31) | Borrow live parent and nonempty relative directory path. Return independently owned anchor. Absolute child paths are invalid. |
| [`fvm_port_anchor_close`](../ports/anchor_port.h#L36) | Consume anchor after every concurrent borrower finishes. NULL is harmless. |
| [`fvm_port_anchor_info_at`](../ports/anchor_port.h#L43) | Borrow parent/path/output. Copy metadata on success; no directory cursor changes. |
| [`fvm_port_file_open_at`](../ports/anchor_port.h#L62) | Borrow parent/path/mode/output. Modes r/w/wx/rw follow ordinary file-port rules; success transfers an independently positioned file. Close through ordinary file API. |
| [`fvm_port_directory_open_at`](../ports/anchor_port.h#L69) | Borrow parent/path/output. Transfer fresh bounded directory cursor, closed through ordinary directory API. Two cursors do not share a native offset. |

All operations can block and are native-only: no VM/GC callbacks. Caller keeps
parent alive through the complete call; close is not concurrent cancellation.
Different returned file/cursor objects can be used independently. Same-object
operations retain the ordinary external synchronization contract. Parent sharing
needs no seek lock. Relative paths permit native dot/dot-dot/symlink traversal;
absolute children are refused. Paths/names retain the existing port bounds.

Bad pointers/path/mode yield INVALID_ARGUMENT, allocation failure OUT_OF_MEMORY,
native failure ERROR; path/name representation or absent backend can yield
UNSUPPORTED. Handle-producing failures clear output. Metadata changes only on
success. Native errors retain errno where applicable; no transactional child-name
snapshot is promised. Directory identity survives rename, child entries may change.

File state/path storage is allocated before destructive opening. POSIX defers
truncation until fdopen succeeds; late fdopen failure closes its real descriptor
and preserves an existing file. New exclusive/nonexclusive creation can leave an
empty file after a later failure. The API never unlinks by a potentially reused
pathname to simulate rollback. Umask and ordinary host permissions still apply.

POSIX uses openat/fstatat and newly opened fdopendir cursors, with close-on-exec.
Directories require read permission. Windows explicitly returns UNSUPPORTED for
new operations; it does not emulate anchors by path concatenation. Existing
absolute file/directory APIs retain their ABI and behavior.

Linux x64 real-port acceptance covers rename with old-name decoys, metadata,
independent cursors, eight OS owners/1024 reads, exclusive creation and injected
late stdio failure. DisVM tests cover reference ownership through FORKNS,
rebind/unmount, imports and retirement. macOS/Android POSIX and a future Windows
backend need native acceptance; this does not close a Flash compatibility stage.
No external runtime library or tool is introduced.

[DisVM integration](../../disVM/runtime/DIRECTORY-ANCHORS.md),
[Russian contract](DIRECTORY_ANCHORS.ru.md).

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_port_anchor_close` | function | [ports/anchor_port.h:36](../ports/anchor_port.h#L36) |
| `fvm_port_anchor_info_at` | function | [ports/anchor_port.h:43](../ports/anchor_port.h#L43) |
| `fvm_port_anchor_open` | function | [ports/anchor_port.h:24](../ports/anchor_port.h#L24) |
| `fvm_port_anchor_open_at` | function | [ports/anchor_port.h:31](../ports/anchor_port.h#L31) |
| `fvm_port_directory_open_at` | function | [ports/anchor_port.h:69](../ports/anchor_port.h#L69) |
| `fvm_port_file_open_at` | function | [ports/anchor_port.h:62](../ports/anchor_port.h#L62) |
<!-- source-links:end -->
