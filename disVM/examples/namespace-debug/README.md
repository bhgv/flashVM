# Inspect shared and copied namespaces

`main.b` is an executable debugger example. It binds source-a to visible, spawns
one shared and one FORKNS/chdir child, and retains both at a first-chance error.
After an explicit stdin permit the parent rebinds visible to source-b while the
shared child stays frozen. The copied child keeps its original group/cwd.
A second permit lets the parent finish after the debugger terminates the children.
[Russian](README.ru.md), [protocol and field table](../../runtime/NAMESPACE-DEBUG.md).

The example intentionally raises managed errors and must run under the built-in
debugger. Its errors have no external handler; continue would make them ordinary
unhandled application errors. Use terminate in this tutorial. Debug traffic uses
separate inherited private pipes, not the stdin permits or guest channels.
Children retain their original frames/groups until release; native cleanup and
retirement own final reference release. No raw pointers cross the boundary.

From repository root after building the console/bootstrap compiler:

```sh
python3 disVM/runtime/test_namespace_debug.py build/disvm-opt/disvm build/disvm-opt/bootstrap/limbo-bootstrap disVM/upstream/module
```

The driver prepares real directories, compiles `main.b` with the bundled Sys and
Processctl declarations, launches the private-pipe debugger and checks all four
interpreter/JIT/GC-poll combinations. It reconstructs long Unicode/quoted paths
across128-byte chunks, enumerates targets, mutates the shared group and requires
stale-revision refusal plus copied-group isolation. It then terminates retained
children and checks normal retirement. An additional test-linked omission ignores
revision and must expose the actual new `/source-b` bytes at an old revision;
crash, timeout or a parser error does not count as detection.

Expected driver lines: four `PASS namespace debugger ...` and two
`DETECTED mixed-revision real source ...`. Linux x64 native/TSan are supported
checks; architecture CI remains separate evidence. Python/compiler/relink tools
are development dependencies, not VM runtime dependencies.

| API | Purpose and ownership |
| --- | --- |
| Sys.bind | Install/replace a held directory binding; group retains its resource. |
| Sys.pctl(FORKNS) | Detach copied native refs before private chdir. |
| Sys.chdir | Acquire cwd/import identity within the copied group. |
| Sys.read / fildes | Borrow rooted stdin FD for explicit host permit. |
| Processctl.pid | Publish scalar identity through ready channel. |
| raise | Retain original first-chance frame until explicit debugger release. |
| namespace / namespace-read | Copy native state under group lock at one revision; no persistent statistics. |
| terminate | Source owner unwinds/retire normally; no foreign thread suspension. |

[Namespace ownership](../../runtime/DIRECTORY-ANCHORS.md),
[debugger](../../runtime/DEBUGGER.md), [native API](../../runtime/vfs/observation.h).
No full Chan/mount/provider or memory-byte accounting is implied.
