# Native files and directories — v0.1650

[Captured frames, packed tuples and old-owner validation](SYS_CAPTURE.md) now apply
to the native metadata adapters; OS filesystem behavior stays in ports.

[Русский](FILESYSTEM.ru.md) · [Runtime](README.md) · [Runnable example](../examples/filesystem/README.md)

`Sys.stat`, `Sys.fstat` and `Sys.dirread` now use the C89
[filesystem port](../../flashVM/ports/filesystem_port.h). DisVM only validates
language arguments, maps metadata to `Sys.Dir`, and manages VM roots. No native
filesystem headers or directory handles cross that boundary. Both standalone
DisVM and the FlashVM-linked editor compile the selected port.

| Target | Implementation | Native behavior |
| --- | --- | --- |
| Linux | [linux/filesystem.c](../../flashVM/ports/linux/filesystem.c) | POSIX stat/fstat, descriptor-relative directory entries |
| macOS | [mac/filesystem.c](../../flashVM/ports/mac/filesystem.c) | Same shared POSIX mechanics, selected separately |
| Android | [android/filesystem.c](../../flashVM/ports/android/filesystem.c) | Ordinary permitted app/native paths; no SAF or content-provider bridge |
| Windows | [windows/filesystem.c](../../flashVM/ports/windows/filesystem.c) | UTF-8 to UTF-16, CRT streams, retained metadata handle and FindFirst/Next cursor |

Linux/macOS/Android share [POSIX mechanics](../../flashVM/ports/filesystem_posix.inc)
inside ports; [file_port.c](../../flashVM/ports/file_port.c) delegates opening and
path validation to that selected implementation. `content://` and other URI
paths return unsupported before ordinary open, including destructive write mode.
Paths and entry names must be valid UTF-8; no byte replacement or truncation occurs.
A basename longer than 4095 bytes is unsupported. Windows currently also rejects
paths containing `://`; use normal drive/UNC spelling, not `C://...`.

## Language contracts

- `sys->stat(path)` returns `(0, record)` or `(-1, empty record)` and an error
  string. Paths containing embedded NUL are rejected before native conversion.
- `sys->fstat(fd)` queries the actual open file/directory. It preserves both
  logical file cursors. The captured basename is only a display name: POSIX
  rename/unlink does not change the identity being queried. Windows rename/delete
  availability depends on the CRT stream sharing policy. Standard descriptors
  0–2 do not yet support this metadata call.
- `sys->open(directory, Sys->OREAD)` owns a directory cursor. `dirread` returns
  `(n, array of Sys.Dir)` in batches of at most 32, `(0, nil)` at EOF, or
  `(-1, nil)` on error. Dot entries are excluded. Ordinary `read`/`seek` on this
  descriptor fail. Files and directories share the existing 29-slot quota.
- Dropping the last `ref Sys->FD` closes its native owner. Arrays and record
  strings remain independent VM-owned values. Old result roots are cleared before
  destruction; a protected builder releases native batches on VM allocation errors.
- Actual Inferno `Readdir` now enumerates and sorts directories through these
  services. Enumeration is not an atomic snapshot. An invalid UTF-8 name, removed
  entry or dangling symlink can fail the whole native batch. Such a cursor becomes
  permanently failed; reopen explicitly rather than silently skipping entries.

File length is preserved as Dis `big`; lengths beyond signed 64-bit range fail.
Native inode/file index and device/volume form the available identity fields;
`qid.vers` derives from modification seconds and is not a collision-free revision.
Times are signed Unix seconds; permissions contain the low nine bits plus DMDIR.
POSIX uid/gid are decimal IDs. Windows uid/gid are empty and permissions are based
on read-only attributes, not Windows ACL interpretation. `muid` is empty.

## Portable API ownership

All declarations below are in [filesystem_port.h](../../flashVM/ports/filesystem_port.h).
Paths and output pointers are borrowed. Metadata outputs change only on success;
port calls require external synchronization and may block their calling thread.

| Function | Result, lifetime and failure behavior |
| --- | --- |
| [`fvm_port_path_info`](../../flashVM/ports/filesystem_port.h#L48) | Follow links and copy path metadata; no handle escapes. |
| [`fvm_port_file_info`](../../flashVM/ports/filesystem_port.h#L43) | Copy metadata from the borrowed live stream, preserving positions. |
| [`fvm_port_directory_open`](../../flashVM/ports/filesystem_port.h#L53) | Transfer one owned cursor on success; output unchanged on failure. |
| [`fvm_port_directory_info`](../../flashVM/ports/filesystem_port.h#L57) | Copy metadata from the retained directory handle. |
| [`fvm_port_directory_read`](../../flashVM/ports/filesystem_port.h#L63) | Transfer a bounded native batch; NULL/0 at EOF. Failure invalidates the cursor and preserves outputs. |
| [`fvm_port_directory_entries_free`](../../flashVM/ports/filesystem_port.h#L67) | Release only a native batch, including NULL; never pass a VM array. |
| [`fvm_port_directory_abort`](../../flashVM/ports/filesystem_port.h#L71) | Mark a cursor failed after caller-side publication failure. |
| [`fvm_port_directory_close`](../../flashVM/ports/filesystem_port.h#L75) | Release the owned cursor; NULL is accepted. |

`FVM_FileNumber64` splits exact 64-bit bits into high/low 32-bit words without
introducing C99 integer types into the public embedding ABI. Native implementation
headers and descriptor extraction are private to ports.

## Verification and remaining work

The executable client checks Unicode names, embedded NUL data, a sparse
4294967303-byte file, three directory batches, actual Readdir sorting, EOF, error
outputs and descriptor reuse. Both dispatch modes run at quanta 1 and 2048.
A separate C89 fixture checks rename/unlink identity and sticky partial failures;
it also runs with address/undefined-behavior instrumentation on Linux.

Windows, macOS and Android sources are selected by CMake; native device/toolchain
acceptance remains open. Calls currently block the single VM owner: this series
does not add background I/O, cancellation, SAF/JNI, namespaces or `/prog` debugger
services. Those are separate work toward a responsive IDE and broker.
