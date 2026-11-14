# Directory anchors under native owners

This executable example keeps cwd/import resources alive while four native
threads detach namespace groups and another owner drops its references. It is
useful when an application changes mounts while existing workers keep using
their working directory. [Russian](README.ru.md),
[full contract](../../runtime/DIRECTORY-ANCHORS.md).

`main.b` binds source-a at visible and source-b at other. Each child uses FORKNS,
changes cwd to visible and opens two independent directory cursors. The parent
removes its bindings before releasing children. Each child covers its cwd with
another bind, checks held reads/imports/create, then leaves cwd and observes the
new virtual route. Optional source-filter unmount checks actual object identity.
Retained provider code stays valid after namespace resources are dropped.

FD locals are managed references and are explicitly cleared when no longer
needed; namespace groups retire with their owners. Channels transfer scalar
ready/start/PID/results only. No cross-heap/native pointers or global stop.
`Rename` is zero for ordinary execution. The development driver compiles a second
variant, waits for readiness, actually renames resources and installs old-name
decoys before permitting work; no external controller is a runtime dependency.

From repository root, with the configured console build and bootstrap compiler:

```sh
mkdir -p build/anchor-example/source-a build/anchor-example/source-b
mkdir -p build/anchor-example/visible build/anchor-example/other
printf A > build/anchor-example/source-a/token
printf B > build/anchor-example/source-b/token
build/disvm-opt/bootstrap/limbo-bootstrap -I disVM/upstream/module -I disVM/examples/namespace-load -o build/anchor-example/source-a/provider.dis disVM/examples/namespace-load/provider-a.b
build/disvm-opt/bootstrap/limbo-bootstrap -I disVM/upstream/module -I disVM/examples/namespace-load -o build/anchor-example/source-b/provider.dis disVM/examples/namespace-load/provider-b.b
build/disvm-opt/bootstrap/limbo-bootstrap -I disVM/upstream/module -I disVM/runtime -I disVM/examples/directory-anchors -o build/anchor-example/main.dis disVM/examples/directory-anchors/main.b
(cd build/anchor-example && ../disvm-opt/disvm --jit main.dis)
python3 disVM/runtime/test_directory_anchors.py build/disvm-opt/disvm build/disvm-opt/bootstrap/limbo-bootstrap disVM/upstream/module
```

Expected ordinary output: `ANCHORSDONE workers=4 bad=0 cwd=A route=B module=11`.
The driver also requires zero live native anchors at shutdown and detects a
controlled real pathname fallback. Both dispatch modes and GC polls are checked.
The runtime needs no installed compiler/Python; these are build/test tools.

| API | Use and ownership |
| --- | --- |
| Sys.pctl / chdir | Copy group references, acquire held cwd; no host process chdir. |
| Sys.bind / unmount | Change exact MREPL route; existing cwd/cursors survive. |
| Sys.open / read | Borrow local rooted FD while reading held directory child. |
| Sys.create / write | Independently own output until cleared; verify original resource. |
| Sys.dirread | Consume each cursor's own bounded batch; inspect metadata. |
| Processctl.pid / killwait | Identify children and join their native retirement. |
| load | Retain provider module instance and its original code. |

See [Sys namespace contracts](../../runtime/VFS-NAMESPACE.md),
[module loading](../../runtime/MODULE-NAMESPACE.md),
[source resource contracts](../../runtime/vfs/resource.h).
Linux x64 is tested. Full mount-parent walk, regular-file identity through its
own rename and non-Linux runtime adapters remain outside this example.
