# Compiler/IDE series 0.1528: acceptance and remaining work

[Русский](SERIES_0_5_0.ru.md) · [Signatures](../../compiler/limbo/frontend/SIGNATURES.md) ·
[ReplaceAll](REPLACE_ALL.md) · [UTF-8 spans](UTF8_SPANS.md)

Base: published flashVM master e3ed60d63bd7c88472471e480604551e15e6092b
(v0.1527). Work is on a local work branch, exported as one format-patch series;
no master commit or push is performed. All commits use bhgv <bhgv@bhgv.bhgv>.

## Executed results on Linux AMD64

| Check | Result |
| --- | --- |
| Root source-built Coco/R | 1/1 |
| AS1/2/3 and Limbo frontends/generators | 40/40 |
| FIR regressions | 14/14 |
| Limbo document service, examples, admission and real FIR navigation | 27/27 |
| Python host development backend | 30/30 |
| ReplaceAll demo selected by root --demo | Both reference modes |
| Detached whole Limbo language project, UBSan | 16/16 |
| Local source-built Limbo bootstrap → production idecore.dis/.sbl | Built |
| Our Coco/R Limbo parser → new idecore.b | Accepted syntax |
| Our .m generator → new idecore.m catalog/signature | Accepted |

The detached export includes the language's own examples/tests, with explicit
Coco/R executable and frames outside it; no hidden sibling frontend dependency.
The standalone suite lacks the root-only cross-AS tests, explaining 16 vs 40.
No UBSan diagnostics were emitted with -fno-sanitize-recover=all.

Root acceptance used a matching explicitly selected reference compiler/module/emu
trio (InferNode abb1b6860238f89eb5a10125e561eff4e0a34836), while production compilation
was separately checked with our source-built tools/limbo-bootstrap. Reference
`-c1` execution is not acceptance of our extracted disVM full host/JIT integration.
The two source-navigation tests run the actual FIR source compiler, check its
retained JSON/source, then execute the same Limbo example client in the reference
VM. Its test transport is Python; the client/service product is Limbo bytecode.

## Reference VM observation retained

Before finalizing the large-string fixture, `huge += huge` after the bulk rendering
workload crashed the older reference emulator in -c1, with “fault while holding
1 lock(s): dereference of nil (proc dis)”. Eight diagnostic reproductions failed
in -c1 and passed in -c0; the truncated probe failed BEFORE sending that huge
string to the document service. Added print calls changed the outcome. This is
consistent with the earlier [reference limitation](REFERENCE_RUNTIME.md), but
its full native root cause was not established in this series.

The accepted regression constructs the SAME exact-size source with a byte array,
retaining expansion rejection, 524288-match shrink and Undo checks. It does not
weaken assertions or retry failures into success. An opt-in diagnostic
[probe](tests/reproduce_reference_concat.py) records both modes and raw stderr;
with the final workload/layout both probe modes passed, so it is not a reliable
standalone reproducer. No runtime fix is claimed. This external reference is not
the pinned bhgv experiment snapshot; no defect was proven/fixed in the bundled
upstream files, whose manifests remain unchanged. No upstream repository is pushed.

## Boundaries

The own compiler now has structured function views and its first fixed-parameter
binding validation; this is not full alias/type/include/layout resolution or full
own Limbo→Dis compilation. AS scalar guards do not supply native inter-VM loading,
GC/capability marshaling, Dis structural signatures or lossless big handling.
Original source selection is not optimized/linked/JIT-PC relocation. Bulk editing
is one-document state mutation, not file persistence or a multi-document transaction.
The portable service still has no rendered widget or complete IDE application.

FlashVM VM, linker, DisVM imported sources and JIT backends were not changed.
The full FlashVM suite was not rerun; the previously recorded four baseline failures
are not resolved or reclassified by this compiler/IDE series. Physical Android,
ARM64 and RISC-V execution were not tested here.
