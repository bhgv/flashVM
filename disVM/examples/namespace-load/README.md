# Concurrent namespace module loading

The example loads providers11/22 through the same virtual name in eight native
owners. It checks private MP, `$self`, bundled bindings, retained old modules,
chdir/NEWNS and invalid names. Launch cwd intentionally differs from entry directory.

From repository root with a supported Linux64 build:

```sh
mkdir -p namespace-load-work/package/source-a namespace-load-work/package/source-b namespace-load-work/package/visible
build/disvm-opt/bootstrap/limbo-bootstrap -I disVM/upstream/module -I disVM/examples/namespace-load -o namespace-load-work/package/source-a/provider.dis disVM/examples/namespace-load/provider-a.b
build/disvm-opt/bootstrap/limbo-bootstrap -I disVM/upstream/module -I disVM/examples/namespace-load -o namespace-load-work/package/source-b/provider.dis disVM/examples/namespace-load/provider-b.b
cp namespace-load-work/package/source-a/provider.dis namespace-load-work/package/visible/provider.dis
cp namespace-load-work/package/source-a/provider.dis namespace-load-work/package/bufio.dis
build/disvm-opt/bootstrap/limbo-bootstrap -I disVM/upstream/module -I disVM/module -I disVM/examples/namespace-load -o namespace-load-work/package/main.dis disVM/examples/namespace-load/main.b
cd namespace-load-work
../build/disvm-opt/disvm --jit package/main.dis
```

Expected: `NAMESPACELOAD workers=8 loads=192 providers=11,22 root=11 old=11 bundled=22`.
`--no-jit` uses the same permanent native-thread model. No installed modules,
debugger service or external runtime libraries are required.

The development driver additionally verifies real cold/warm held opens, atomic
replacement at equal size/time, metadata omission and unrepresentable identity.
See [exact cache and VFS limits](../../runtime/MODULE-NAMESPACE.md),
[Russian](README.ru.md).
