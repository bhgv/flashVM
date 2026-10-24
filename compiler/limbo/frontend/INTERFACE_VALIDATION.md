# Interface declaration acceptance, 0.6.6

Checked on 2026-10-24 against Inferno experiment
`0a0a007ebf6c0276ada93367dde149df06efce12`.

- External coco-r-limbo C-host/C-output generator: 12 default CTest targets pass;
  with the explicitly selected upstream checkout, all 13 targets pass.
- External coco-r-C generator: all 12 default targets pass.
- ASan+UBSan: all 12 default targets pass. The external upstream corpus also passes
  with the sanitized inspector. Leak detection is disabled; owned-catalog allocation
  tests independently count exact allocation/release balance on each injected failure.
- Detached/renamed compiler/limbo plus LICENSE: builds independently and all 12
  default tests pass. NativeExample.m is accepted with six expected catalog entries.
- Real upstream corpus: 153 explicit `.m` files accepted; runt.m deliberately
  rejected because it is an `implement nothing` include aggregator. No includes
  are expanded and no module implementations are imported/linked by these checks.
- Direct function names from Sys and Math match 43 and 68 checked-in native Runtab
  names respectively. This is **111 export-name checks**, not signature/layout/map parity.
- C API tests retain owned source/names across caller overwrite, 32 repeated
  malformed/unsupported/duplicate parse cycles, 16384-entry exhaustion and recovery.
  Empty catalogs, optional diagnostics, invalid metadata/index/scope and unchanged
  output contracts are checked.
- Independent source-order lookup oracle covers 20 modules with 25 functions each,
  lexical-vs-numeric name ordering, shared names across scopes, missing names,
  case-sensitive identities and non-ASCII names.
- Fault injection covers catalog owner, per-node scope index, declaration-array
  growth, every copied name and sorted-key allocation. Older owner/lookup remains
  valid; each failure and recovery returns to the exact prior allocation balance,
  and final destruction returns the catalog-store balance to zero. Tree/scanner
  allocation is outside this injection (the separate tree tests remain present).

No full compiler, include/type/import resolver, ADT field/method catalog, C emitter,
target profile, native function frame/struct layout, type-signature hash, GC map,
Dis/JIT execution or real-device acceptance is claimed. Existing syntax/tree limits
and external serialization requirements remain. New catalog code is strict C99;
all language recognition remains in the existing authoritative Coco/R grammar.
