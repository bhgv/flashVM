# Параллельная загрузка модулей через namespace

Пример загружает providers11/22 по одному virtual name в восьми native owners.
Проверяет private MP, `$self`, bundled bindings, сохранность старого code,
chdir/NEWNS и неверные имена. Launch cwd намеренно отличается от entry directory.

Из корня репозитория с поддерживаемой Linux64-сборкой:

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

Ожидается: `NAMESPACELOAD workers=8 loads=192 providers=11,22 root=11 old=11 bundled=22`.
`--no-jit` использует ту же модель постоянных системных потоков. Установленные
модули, debugger service и внешние runtime libraries не требуются.

Development driver дополнительно проверяет настоящие cold/warm opens, atomic
replacement с равными size/time, metadata omission и слишком широкую identity.
См. [точные cache/VFS-границы](../../runtime/MODULE-NAMESPACE.ru.md),
[English](README.md).
