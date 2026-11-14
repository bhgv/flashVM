# Remove names without invalidating opened resources

This executable example uses four permanent native owners. Each detaches a
namespace, enters a directory binding and holds an opened file and loaded provider.
It removes the names and still reads/calls the original resources, then verifies
that fresh image loading fails. Empty directories and file aliases are removed;
nonempty/dot/root/NUL paths refuse. A shared-name race has exactly one winner.
[Russian](README.ru.md), [full contract](../../runtime/NAMESPACE-REMOVE.md).

`Rename=0` runs without a host rename controller. The development driver also
compiles Rename=1, waits for all opened resources, performs an actual parent
rename and installs decoys at the old names before permitting removal. Debug/test
observation uses only scalar counters. The shared return array exercises ordinary
result-cell guards; a probe rejects repeated effect calls and requires zero live
anchors after native retirement. No global stop or cross-heap native pointer.

Rooted FD locals are cleared after read; Modlinks keep code/private MP until guest
retirement. Namespace group/plan references close independently at final release.
All host effects take place in an owned temporary directory. The omission control
performs one actual stale-path deletion, then requires detection and normal exit.
It never runs against user files.

From repository root after building the console/bootstrap compiler:

```sh
python3 disVM/runtime/test_namespace_remove.py build/disvm-opt/disvm build/disvm-opt/bootstrap/limbo-bootstrap disVM/upstream/module
```

The driver compiles `main.b` and the existing namespace-load provider-a using
bundled declarations, prepares all fixtures and checks interpreter/JIT and both
GC polls, with/without rename. Expected eight `PASS namespace remove ...` lines
and two `DETECTED actual decoy deletion ...` lines. It checks actual file states,
old-name decoys, alias target preservation, one-winner removal and joined owners.
Compiler/Python/relink are development dependencies only, not runtime tools.

| API | Purpose and ownership |
| --- | --- |
| Sys.pctl(FORKNS), chdir | Detach native refs and hold cwd/import resource. |
| Sys.bind, unmount | Bind directories/file sources; alias deletion preserves underlying target. |
| Sys.open, read | Hold rooted FD across unlink, then clear local ref. |
| Sys.stat | Check selected name is absent; returns owned metadata only on success. |
| Sys.remove | Perform one native effect; only scalar result publication retries. |
| load | Retain old Module, then refuse reopening removed image. |
| Processctl.pid, killwait | Identify and join original native owners. |

[Directory lifetime](../../runtime/DIRECTORY-ANCHORS.md),
[portable remove](../../../flashVM/doc/DIRECTORY_REMOVE.md).
Linux x64 is executable evidence; non-Linux adapters/physical acceptance, full
Chan/provider union mounts, and deletion of a directory binding itself remain open.
