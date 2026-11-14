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
| `unmount(source, on)` | Remove a replacement only if its frozen native source matches |
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

A path plan copies virtual and native names plus group revision under its mutex.
Host I/O uses only the native copy, after unlocking and leaving managed admission
through the existing native I/O adapter. Concurrent mutations cannot alter an
in-flight open/stat path. No group pointer is used while outside managed admission.

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

This is **host-path routing**, not the complete Inferno Chan/provider VFS or a
security sandbox. Native path strings do not preserve resource identity through
host rename/unlink. The host still follows symlinks. Virtual `.`/`..` normalization
is lexical, including before bind lookup; it is not host symlink traversal.
Paths are limited to4096 UTF-8 bytes including NUL. Empty and oversized paths fail.
Host filesystem changes between validation and later access remain possible.
The cwd is a virtual spelling rather than an open Chan: replacing a prefix
covering cwd changes subsequent relative lookup. Stable directory identity
through rebind/rename requires the later provider-handle layer.

The module loader still uses its existing native resource lookup/cache, outside
this namespace; namespace-aware loading needs provider identity in cache keys.
Standard descriptors and the explicitly enabled test-control node also retain
their existing routes. No FD-group, environment-variable or credential isolation
is provided by FORKNS/NEWNS. Existing descriptors may reach outside a new root.

Union mounts, mount transport, device/socket/channel providers, provider handles,
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
