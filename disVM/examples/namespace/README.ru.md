# Общие и отделённые namespace

Исполняемый `main.b` проверяет стандартный Sys с семью native-потомками: общий
namespace и копию, общий cwd, NEWNS root, MREPL bind/unmount, перенаправленные
open/stat/create, сохранность ранее открытого FD и отказ неподдержанным flags.
Четверо потомков пишут builtin returns прямо в общий ADT, пока родитель читает
его. Используется обязательная многопоточность без exclusive-режима.

Из корня репозитория с поддерживаемой Linux64-сборкой:

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

Ожидается строка: `NAMESPACE shared=B copied=A descriptor=V create=C root=A restored=V`.
Программа создаёт `source-a/created`. `--no-jit` проверяет interpreter с теми же
системными потоками. Автотест создаёт отдельные временные fixtures и проверяет
оба режима/GC poll в обычной и TSan-сборках.

Это маршрутизация host-путей, не security sandbox и не полная Chan VFS. Symlinks
и ранее открытые FD могут выходить за NEWNS root. См. точные
[границы реализации](../../runtime/VFS-NAMESPACE.ru.md) и [English](README.md).
