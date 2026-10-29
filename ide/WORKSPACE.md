# Dis workspace and compiler-job ownership

[Interface](dis/bridge/ideworkspace.m) · [Service](dis/bridge/ideworkspace.b) ·
[Callable tutorial](dis/examples/workspace/README.md) · [Native module namespace](../disVM/MODULE-STORE.md)

`ideworkspace` owns up to 16 independent `idecore` document/history services.
It loads no Sys, Draw, Tk or UI module. AS remains responsible for presentation,
colouring and effects; document identity, edits/history, dirty-close policy and
compilation-result admission stay on Dis channels.

## Document protocol

Open accepts a canonical logical source path and text, returning a stable
workspace-lifetime document ID. Duplicate names and unsafe/noncanonical path
segments are rejected. Names are valid Unicode, at most 255 UTF-8 bytes.
List returns fresh metadata arrays/ADT records. Clients cannot mutate private
metadata or obtain the document owners' request endpoints.

Command forwards an `idecore` request after privately replacing its reply
channel. Inspect both the workspace status and `core.status`: a successful
forward is not a successful edit. Open/Close/Shutdown cannot be smuggled through
this forwarding operation. Existing Unicode coordinates, revision checks,
undo/redo and bounded history remain in the actual core implementation.

Close checks exact revision and dirty state. Shutdown first validates every
document before stopping any owner. Dirty work needs an explicit force flag;
a rejected shutdown leaves all documents alive. Every private shutdown reply
is consumed before the workspace emits its terminal acknowledgement.

## Real compiler workers, no fabricated builds

BeginBuild snapshots immutable source text, revision, target (`dis`, `swf` or
`fir`) and a unique monotonically increasing ticket. A newer job supersedes the
older ticket. The owner never blocks while an external compiler performs work:
the worker receives its snapshot, compiles separately, then submits FinishBuild.

FinishBuild validates ticket, original revision and current document revision
before admitting any binary. Failed compilations return BuildFailed with bounded
plain diagnostics. Successful results are copied before acknowledgement, limited
to 2 MiB per artifact and 8 MiB retained across the workspace. Client input arrays
must remain unchanged until acknowledgement. Artifact replies copy bytes again.
No mutable array crosses this boundary by shared ownership.

A pending job with another target cannot relabel a previous successful artifact.
A failed new build leaves the last successful artifact available only while its
source revision is still current. Editing/undo/redo advances the revision, so
Artifact then returns Conflict even if the resulting text happens to match.
Document closure removes its retained artifact budget. Diagnostic text is limited
to 65536 Unicode characters. Document/history budgets remain idecore's per-document
limits, multiplied by the finite document count; no arbitrary project size is
admitted. Allocation failure is still the VM's exception boundary.

An optional [native compiler worker](COMPILER-SERVICE.md) now connects these
channels to our source/FIR/SWL libraries on the actual DisVM. Automatic native-store
publication is not wired into these channels yet. This service coordinates real jobs/results; it does not
claim to compile arbitrary AS/Limbo or launch the complete IDE. The native byte
store and Limbo workspace have separate owners and are not a shared native ABI.

## Validation and build

The production service and test execute through actual Dis rendezvous channels
in matching external reference-runtime modes 0/1. Checks cover private document
state, metadata/byte isolation, stale/superseded jobs, target preservation,
failed builds, artifact/document quotas and dirty/terminal teardown. The own DisVM now also runs workspace and compiler-service acceptance in actual
interpreter/native JIT modes; this does not establish physical foreign-CPU acceptance. The source bootstrap separately compiles the
service and its debug data without an installed external runtime.

Normal root IDE builds include `ideworkspace.dis`, `.sbl` and the callable
`workspace-example.dis` tutorial. Packages contain headers, commented source and
this protocol guide. Optional reference tests require an explicitly matching
compiler/header/emulator trio, as the existing IDE reference tests do.

## По-русски

Владелец рабочего пространства и сборок написан на Limbo. Каждый документ имеет
свой настоящий idecore с историей. Задание сборки получает текст, ревизию и билет;
результат принимается только для неизменённой ревизии и последнего билета.
Поздний результат не затирает новый, а редактирование не выдаёт старый бинарник
за текущий. Массивы копируются на входе/выходе, квоты проверяются до публикации.
Закрытие с несохранённым текстом требует явного force. Это сервисная часть IDE;
панели полного GUI ещё не завершены. Нативный [compiler worker](COMPILER-SERVICE.ru.md)
и workspace теперь исполняются на нашем DisVM; автоматического запуска артефактов нет.
