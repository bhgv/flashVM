# Module loading uses the guest namespace

Directory resources are now retained through lookup/open/parse. The absolute
name below is display provenance and may become stale after rename; it is never
reopened for anchored I/O. See [current directory contract](DIRECTORY-ANCHORS.md).

File imports now resolve the current group's namespace **once**, before cache
lookup and native release. The cache key, open and parser receive the same copied
absolute display spelling and held resource. A concurrent bind cannot redirect an in-flight read.
The loader owns its opened descriptor through metadata/read/close; metadata comes
from that object, even after its pathname has been replaced. There is no global
execution exclusion, thread migration, saved-REG restore or external service.

## Initial environment and inheritance

The startup controller creates a native namespace before loading the entry image.
The first guest retains that group; spawned guests share or detach it as before.
The controller's own reference is released after all native owners join, including
startup-error cleanup. Ordinary Sys paths initially use host cwd. For compatibility
with existing packaged programs, the group's initial relative-import directory
is the entry package directory. Both are explicit per-group state.

`FORKNS` copies the import directory and bindings coherently. Successful `chdir`
updates cwd and relative-import directory together, under one group mutex.
`NEWNS` uses resolved cwd as native root and `/` for both virtual directories;
it has no inherited bundled-resource fallback.

The existing nine bundled names under `/dis/lib/` are ordinary initial file
bindings to entry-package siblings. `Sys.open/stat` and `load` see the same
binding; user MREPL can replace it. The old special mapping in `kopen` is removed.
Only the existing names are installed; directory listings of their virtual
parents are not synthesized. Builtins beginning `$` retain their global registry
contract; `$self` still links this instance's code/MP. Embedded NUL is rejected
before builtin/self/file classification, and nil-H/empty names fail normally.

## Cache provenance and retained instances

The old hosted `kdirfstat` supplied length but zero identity and time, so an old
Module could be accepted after a different file replaced the same path. It now
copies real inode/file identity, device, modification seconds and length from
`fvm_port_file_info` under that descriptor's gate. Cache checks use those fields.
Length occupies private loader Qid version; this does not change Sys.Dir or wire
formats. Device high bits use the retained type field; unrepresentable identities
decline reuse. A private Qid marker also prevents a narrowed old Module from
matching a later representable file. A declined cache still permits a fresh load.

Module.path now records the frozen absolute host spelling. Debugger tests require
that exact provenance; they do not relax checks to basename-only matching.
Different spellings of the same host object may have separate cache entries.
The same spelling can retain several live versions after atomic replacement.
Each Modlink keeps private MP and its original code; binding/replacement affects
future loading without changing retained instances or their function quarantine.

Publish compiled images as immutable files with atomic replacement. The existing
port timestamps have second precision; in-place edits preserving identity,
length and modification seconds are not a cache-coherence guarantee. Concurrent
in-place writes are not a whole-image transaction. Full provider revision/content
identity remains separate work, together with full Chan/provider handles. Host
symlinks and lexical dot-dot retain the [namespace boundary](VFS-NAMESPACE.md).

## Evidence and limits

The public example starts outside its entry directory. Eight detached native
owners load two implementations under the same virtual name, perform192 repeated
loads, check private MP through native sleeps and `$self`, and retire. It also
checks bundled-file rebind, retained old code, chdir/NEWNS, missing/nil/NUL names.

A test-linked probe holds a **successful real open**. Another guest rebinds and
loads before release; the host atomically replaces the original path with a
different inode while preserving length and timestamp. Real registry observations
require both cold and warm ownership. The held source receives old code, its next
load receives new code, and old instances remain valid. Four interpreter/JIT/poll
combinations run both cold/warm cases. An omission restores zero metadata and
must produce the exact stale value with normal retirement, not a crash. Another
case injects an identity wider than retained fields, requiring safe cache refusal.

These checks run on Linux x64 ordinarily and under TSan. The mandatory native
suite now has66 workloads including `namespace-load`; new ARM64/RV64 CI remains
a separate gate. Common namespace/cache code uses the existing portable FS/sync
interfaces. Windows/macOS/Android executable runtime adapters are still pending.
No vendored source or serialized ABI was changed. See the
[example](../examples/namespace-load/README.md) and [Russian](MODULE-NAMESPACE.ru.md).
