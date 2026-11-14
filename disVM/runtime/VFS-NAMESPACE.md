# Native namespace groups: first VFS integration

Each resident owns a native reference to a namespace group. `spawn` shares the
parent's group. The group contains its virtual current directory, native root
and replacement bindings. It has its own portable mutex; no global VM exclusion
or stop is introduced. Native group storage contains no managed pointers/GC roots.
Every failed process setup and final retirement releases its reference; children
can outlive their parent. This is functional namespace state, not a permanent
per-object statistics catalog.

## Standard Sys calls

| Call | Implemented behavior |
| --- | --- |
| `pctl(0, nil)` | Return current PID |
| `pctl(FORKNS, nil)` | Detach a coherent copy of root, cwd and bindings |
| `pctl(NEWNS, nil)` | Detach an empty namespace rooted at the resolved current directory; virtual cwd becomes `/` |
| `chdir(path)` | Validate a directory and change group cwd; host process cwd never changes |
| `bind(source, on, MREPL)` | Replace an existing like-kind resource at the exact virtual target |
| `unmount(source, on)` | Remove a replacement only if its full device/inode identity matches |
| `unmount(nil, on)` / empty source | Remove the exact replacement without a source filter |

Other `pctl` flags, combined flags, non-nil `movefd`, union/create/cache bind flags
and absent bindings return `-1` with a guest error. They never pretend success or
partly change the namespace. Existing create restrictions remain: `OWRITE`, optional
`OEXCL`, permission `0666` subject to host umask.

`Sys.open`, `Sys.create` and `Sys.stat` now resolve the caller's namespace. Read,
write, seek, fstat and dirread continue through their existing handles; rebinding
does not redirect already opened descriptors. Longest matching component prefix
wins; `/view` does not match `/view-other`. Sources are resolved once at bind time,
so later source rebinding cannot create recursive alias lookups. More-specific
bindings remain after replacing/removing a parent prefix.

## Concurrency and ownership

A resource plan copies names/revision and retains a directory handle under its
mutex. Host I/O uses that handle and its relative child path after unlocking and
leaving managed admission. Rebind/unmount cannot close an in-flight selection.
No group pointer is used while outside managed admission.

For chdir/bind, validation I/O runs outside the group lock. Commit succeeds only
if the observed revision still matches. Conflicts retry at most eight times, then
return an explicit error. Versions never wrap; exhaustion refuses mutations.
FORKNS copies coherently under the source group's lock. NEWNS takes a coherent
resolved cwd snapshot; subsequent peer changes affect the original group only.
Bindings, copies and references use native allocation and never call the VM while
holding a namespace mutex.
Scalar return publication uses the existing shared-cell guards. If busy, only
publication retries; an already committed namespace change is never repeated.

## Exact boundary of this milestone

Directory resources now preserve identity through host rename and covering
rebind; see [retained directory contract](DIRECTORY-ANCHORS.md). This remains
a subset of Inferno VFS, not a complete Chan/provider implementation or sandbox.
Regular-file bindings retain parent-directory plus basename, not the file itself.
The host follows symlinks. Virtual dot/dot-dot is lexical; leaving cwd's subtree
returns to virtual-root lookup. Paths remain limited to4096 bytes including NUL.
Children can change between validation and access.

The module loader now uses the same frozen namespace plans for cache/open/parse;
see [module namespace and cache provenance](MODULE-NAMESPACE.md). Initially,
relative imports use the entry package directory; chdir updates that group state.
Standard descriptors and the explicitly enabled test-control node also retain
their existing routes. No FD-group, environment-variable or credential isolation
is provided by FORKNS/NEWNS. Existing descriptors may reach outside a new root.

Union mounts, mount transport, device/socket/channel providers, full Chan handles,
namespace inspection API, FD/env/credential groups and dynamic C/Limbo provider
loading remain separate work. The implementation follows the FORKNS/shared-cwd
and NEWNS-rebase semantics inspected in Inferno experiment's
`emu/port/inferno.c`; vendor source is unchanged, and no legacy OS exclusion was
imported. EN/RU source contracts are in `vfs/`.

| Platform | Evidence for this change |
| --- | --- |
| Linux x64 | Real VM tests in interpreter/JIT, both GC polls; normal and TSan builds |
| Linux ARM64 / RV64 | New mandatory `namespace` workload registered; architecture CI required |
| Windows / macOS / Android | Shared namespace core uses libc and the existing portable mutex API; executable runtime adapters are not ported or verified by this change |

The runtime remains self-contained; Python/CMake are development checks, not
installed runtime dependencies. See the [public example](../examples/namespace/README.md)
and [Russian text](VFS-NAMESPACE.ru.md).

Validation: full ordinary67-group regression passed; focused11 groups passed
normally and under TSan, plus12 additional TSan lifecycle/debugger groups. After
the final guarded-result change, namespace/concurrency checks passed again in
both builds. Four direct shared-result writers are checked in both execution
modes/polls. A test-only raw-copy omission produces actual TSan data races in
interpreter and JIT; normal exit/crash/compiler failure is not accepted as proof.
The standalone core also passed ASan/UBSan with leak detection disabled because
of sandbox ptrace restrictions; no leak-coverage claim follows from that run.
