# Explicit binary/debug export — v0.1672

Build with F7, then press **Ctrl+F7** for a binary or **Ctrl+Shift+F7** for original
FIRDBG1. The existing Dis field opens an explicit destination; Enter attempts to
create a **new** file, Escape cancels. Successful export shows the byte count.
Errors leave the destination editable and preserve source, history, caret and
saved revision. Existing files, directories and symlinks are always rejected:
choose another name. Export never silently overwrites the source or an old build.

The suggested binary suffix is the retained artifact's actual `.dis`, `.fir` or
`.swf`, independent of the source path's name. Debug export requires `.firdbg`;
it is original source/FIR information, not Inferno `.sbl` or a final target-PC map.
A successful F7 build is required for the current source revision. The service
rejects stale revisions, absent results, mismatched targets and wrong suffixes
before opening a destination. Plain F7 still writes no file.

## Channel library

[Ideartifact](dis/bridge/ideartifact.m) borrows Sys, Idefileops and a live
Idebuildpanel endpoint. `start` returns nil on nil dependencies, otherwise a FIFO
channel. Export requests carry nonnegative ID, exact revision, expected target,
Binary/Debug part, explicit path and required reply endpoint. Every admitted
request requires consumption of its unique reply. Shutdown is terminal and does
not shut down borrowed services. The editor stops exporter, panel, document and
native channel borrowing in that order on every exit path.

The panel returns independently owned binary/debug arrays. Export binds to the
immutable artifact snapshot admitted by that fetch. A later edit does not cancel
an already admitted write; no atomic transaction spans document editing and disk
I/O. The sequential graphical controller prevents edits while its destination
field is active. Binary and debug files are two separate explicit transactions:
a failure of the second does not remove a successful first file.

[Idefileops.readbytes/writebytes](dis/bridge/idefileops.m) preserve arbitrary bytes,
including zero and invalid UTF-8. Paths are bounded/NUL-free, byte buffers at most
4 MiB, read limits 1..4 MiB, exclusive flag exactly 0/1. Reads return nil on error,
never a partial successful result. Strict UTF-8 `read` retains its existing
contract by validating the returned binary bytes. Ordinary text `write` retains
checked-overwrite semantics; exporter always chooses exclusive binary creation.

Native Sys.create now accepts OWRITE with optional OEXCL and permission 0666.
The shared file port uses POSIX O_CREAT|O_EXCL or Windows _O_CREAT|_O_EXCL through
its selected OS adapter. There is no stat/open gap. Owners are allocated before
creation; a later stream/write error may leave an empty/partial **new** file.
No rollback, rename transaction, fsync/durability or filesystem sandbox is claimed.
Existing objects are never truncated by exclusive export. Native I/O and
compilation block the sole VM owner; no cancellation/background responsiveness.

## Compiler consistency correction

Export acceptance exposed a library/CLI difference for optimized constant-only
Limbo: the in-process backend retained removed temporary value slots although
serialized FIR was identical. Binary emission now serializes/reparses/verifies the
canonical FIR before Dis/SWO emission, matching the CLI's layout and bytes. FIRDBG1
continues to describe the original source. This correction belongs to our C99
compiler service, not shared Inferno interpreter code.

## Verification and example

The native ASan/UBSan fixture checks all 256 byte values, existing file/symlink
preservation, and eight barrier-released OS processes competing for one path
(exactly one winner). It exercises the actual Linux port; other OS implementations
are source adaptations, not physical platform acceptance. LSan is unavailable in
the sandbox. Real Dis acceptance exports all three formats/debug, compares CLI
bytes, runs exported Dis to 42, and checks invalid/stale/target/I/O failures.
Graphical replay tests five source suffixes, explicit destination fields, both
parts, cancellation, existing files, stale results and unchanged source/saved state
in interpreter and JIT modes. These are real bytecode/rendering tests, not physical
window/touch acceptance.

[Every-line-commented tutorial](dis/examples/artifact-export/README.md) ·
[Build panel](BUILD-PANEL.md) · [Compiler service](COMPILER-SERVICE.md) ·
[File primitive](../flashVM/doc/EXCLUSIVE_FILES.md) · [Source](dis/bridge/ideartifact.b).
