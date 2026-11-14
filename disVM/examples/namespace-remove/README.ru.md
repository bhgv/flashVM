# Удаление имён с сохранением открытых ресурсов

Исполняемый пример: четыре native owners делают FORKNS/chdir в directory binding,
держат открытый FD и provider Module. После remove имён FD читается, старый code
работает, новый load отклоняется. Проверяются empty dirs, file aliases, отказ
nonempty/dot/root/NUL и ровно один победитель удаления shared name.
[English](README.md), [контракт](../../runtime/NAMESPACE-REMOVE.ru.md).

Rename=0 не требует host controller. Драйвер собирает Rename=1, ждёт opened
resources, реально переименовывает parent, ставит decoys по старым именам и
разрешает remove. Shared return array использует обычные result guards; probe
отклоняет повторные effects и требует zero live anchors после retirement.
Нет global stop и native pointers между heaps.

Rooted FD очищается после read, Modlinks сохраняют code/private MP до retirement.
Group/plan refs независимо закрываются при final release. Все filesystem effects
в собственном temporary directory. Omission действительно удаляет один decoy по
старому пути; проверяются detection и normal exit. User files не используются.

Из корня после сборки console/bootstrap:

```sh
python3 disVM/runtime/test_namespace_remove.py build/disvm-opt/disvm build/disvm-opt/bootstrap/limbo-bootstrap disVM/upstream/module
```

Драйвер собирает main и namespace-load provider-a с bundled declarations, создаёт
fixtures и проверяет interpreter/JIT, оба GC polls, с/без rename. Ожидаются восемь
`PASS namespace remove ...` и два `DETECTED actual decoy deletion ...`. Проверяются
реальные states files/decoys, сохранность alias target и joined owners.
Compiler/Python/relink — средства разработки, не зависимости runtime.

| API | Назначение и владение |
| --- | --- |
| Sys.pctl(FORKNS), chdir | Копирование refs, удержание cwd/import. |
| Sys.bind, unmount | Directory/file routes; alias removal сохраняет target. |
| Sys.open, read | Rooted FD через unlink, затем очистка local ref. |
| Sys.stat | Проверка отсутствующего имени; metadata только при успехе. |
| Sys.remove | Один native effect; retry только scalar publication. |
| load | Старый Module жив; новый load image отклоняется. |
| Processctl.pid, killwait | PID и ожидание original native owners. |

[Directory lifetime](../../runtime/DIRECTORY-ANCHORS.ru.md),
[portable remove](../../../flashVM/doc/DIRECTORY_REMOVE.ru.md).
Проверен Linux x64. Non-Linux adapters/physical acceptance, полный Chan/provider
union и удаление самого directory binding остаются работой.
