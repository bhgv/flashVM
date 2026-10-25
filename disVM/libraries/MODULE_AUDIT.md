# Sys and selected module audit — v0.1586

[Runtime](../runtime/README.md) · [Executed library coverage](README.md) ·
[Inferno ownership fixes](../SYNC.md) · [Русский](#по-русски)

The audit follows the actual registered native modules and selected Limbo
libraries. This does not import or certify every Inferno service.

| Module/boundary | Checked evidence | Result and remaining boundary |
| --- | --- | --- |
| Sys formatting, tokenize and UTF | Core sys-print, sys-tokenize, sys-tokenize-matrix, sys-utf, token-capacity/retains | Byte-copied scalar/tuple outputs and result ownership; no new defect confirmed in these paths |
| Sys host I/O | File positioning/size/standard streams, runtime file windows; core sys-io/stream/dirread/status/stat/pctl fixtures | The standalone host implements its documented file/stream subset; core emulator/OS fixture coverage does not make unsupported native host calls available |
| Srv | Actual wrappers, cons, newstring and native error protection; 35,200 existing cases plus 24 preflight cases across WORD32/64 | Fixed nil/H output slots and invalid/maximum Tptr references; every failure releases native answers and restores locks |
| Loader snapshots | Actual ifetch/link/tdesc, heap array/maps and protected construction; 286 cases per WORD model | Fixed unaligned output access and publication after old-owner destruction; 108 new alignment/replacement/preflight cases total |
| Loader construction/scalars | Existing loader-runtime and loader-snapshots | Existing allocation failure/retry, descriptor ownership and compiled-export checks passed; the new slot fix is scoped to the three snapshot APIs |
| Math | Existing checked wrappers, numeric primitive descriptors, runtime libraries-check and binary64 checks | Actual selected $Math/libm path; no new Math source defect confirmed |
| Bufio, String, Arg, Readdir | Existing selected library acceptance and corresponding Sys boundaries | Packaged code executes its covered calls; full directory/namespace support is not implied |
| Debug, Dis, Styx, Styxservers | Existing selected module smoke checks and snapshots | Not a complete /prog debugger, remote debug session or running Styx service |

Srv fixes and Loader fixes were committed in their owning Inferno repository,
including sanitizer regressions and parent/guard-removal failure probes. Selected
FlashVM snapshots are refreshed from published commit
`7f6143d6af8be1a9a59669e7a1423a542c85cd88`; no private vendor-only patch is used.

Both Srv and the full 286-case Loader snapshot matrices pass combined ASan/UBSan
in WORD32 and WORD64. Loader fixture call frames/canary images now have stable
storage across native error unwinds. This addresses an ASan stack-shadow report
when the sandbox cannot discover the stack top; sanitizers remain enabled.
The unchanged baseline was checked separately. Parent Loader fails the observed
owner-publication contract; parent Srv fails nil-result access. Removing only
Srv's descriptor guard fails the new invalid-retain checks.

## По-русски

Проверены Sys и реально подключённые модули, с разделением host-реализации,
core-фикстур и Limbo-библиотек. Найдены и отправлены обратно ошибки Srv и трёх
snapshot-API Loader. Sys/Math в проверенных путях не потребовали нового патча.
Наличие Readdir/Styx/Debug в поставке не означает готовности namespace, сервера
Styx и полного /prog-отладчика. Границы остальных Loader API сохранены: нельзя
распространять проверку snapshot-слотов на все методы модуля.

## Follow-up v0.1593: remaining Loader result APIs

The earlier snapshot-only boundary is now extended by published Inferno fix
`74c193724a7352b8fda2724a5cf10029885be68c`: newmod/dnew clear old result owners
before destruction and byte-copy completed pointers; tnew/ext/compile publish
full WORD status without assuming alignment. Nil/H destinations fail before
module mutations. [Upstream regression contract](../upstream/libinterp/tests/README.loader-results.md)
adds 100 WORD32/64 alignment, replacement and nil cases under ASan/UBSan, while
retaining the existing construction/failure/retry suite. This extends result-slot
coverage, not arbitrary malformed-frame or sandbox guarantees.
