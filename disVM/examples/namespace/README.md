# Shared and detached namespaces

The runnable `main.b` exercises standard Sys calls with seven native children:
shared and copied namespaces, shared cwd, NEWNS root, MREPL bind/unmount, routed
open/stat/create, unchanged pre-bind FD and explicit unsupported-flag errors.
Four children write builtin returns directly into a shared ADT while the parent
reads it. It uses the mandatory native-thread runtime; no exclusive mode is needed.

From the repository root on a supported Linux64 runtime build:

```sh
mkdir -p namespace-work/source-a namespace-work/source-b namespace-work/visible
printf A > namespace-work/source-a/token
printf B > namespace-work/source-b/token
printf V > namespace-work/visible/token
build/disvm-opt/bootstrap/limbo-bootstrap -I disVM/upstream/module \
    -o namespace-work/main.dis disVM/examples/namespace/main.b
cd namespace-work
../build/disvm-opt/disvm --jit main.dis
```

Expected line: `NAMESPACE shared=B copied=A descriptor=V create=C root=A restored=V`.
The program creates `source-a/created`. Run `--no-jit` to exercise the interpreter
with the same native-thread model. The automated test creates isolated temporary
fixtures and runs both modes/poll backends, normally and under TSan.

This is host-path routing, not a security sandbox or full Chan VFS. Symlinks and
existing descriptors can reach outside a NEWNS root. See the exact
[implementation boundary](../../runtime/VFS-NAMESPACE.md) and [Russian](README.ru.md).
