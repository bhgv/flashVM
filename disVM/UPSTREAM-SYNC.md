# Inferno experiment / DIS64 / Sys synchronization

Copyright (c) 2026 bhgv

Updated source: `bhgv/Inferno-OS-bhgv`, branch `experiment`, pinned to
`7f2241455734e1142a3254fad9d1ec4f94032ac7` in all four active snapshots:
DisVM, Limbo bootstrap, mk and iyacc. The previous main pin was `ba26accb`;
Inferno `c7c60337` was 347 commits ahead. Notices and selected blobs remain exact.

## Owning fixes and data exchange

- [`9e302aad`](https://github.com/bhgv/Inferno-OS-bhgv/commit/9e302aadf0d8eb5330ea9969ab35b322c182c434)
  regenerates committed `limbo/y.tab.c` with current limbo.y and current iyacc.
  The old artifact accepted `-6` and rejected `-64m/-64l`. The regenerated artifact
  preserves the corrected sentinel/recovery code and byte-identical token header.
  Replaying the new Sys gate failed before this repair and passed afterwards.
- [`FLASHVM-0172`](https://github.com/bhgv/Inferno-OS-bhgv/commit/7f2241455734e1142a3254fad9d1ec4f94032ac7)
  appends the authorized handoff: master native-only policy, exact CI scope,
  NC64/DIS64 distinction, Sys results, the recovered proxy contract and questions
  about native64 network FD and managed signatures. The Inferno-owned journal is
  unchanged. The journal is read during active work; it is not a background agent.

## Host adaptations

Upstream Loader startup now stages descriptors before module publication and
checks instruction coordinates. Its reviewed source fingerprint was updated only
after inspecting that change and the unchanged retain boundaries. interp.h adds
Module metadata/declarations; Heap/Type/color layout is unchanged. The new MP
owner refcount preflight in link.c uses an atomic load in the concurrent host.
These adaptations stay outside upstream; no new global exclusion is introduced.

## Checked boundaries

- Source-built mk and iyacc; actual limbo.y regeneration and exact token header.
- Committed-parser and freshly generated parser compilers produced byte-identical
  ordinary Dis and DIS64 images for all 67 scalar and 241 helper sources.
- New committed-parser compiler, logical cell=4: the owning mixed fixed Sys gate
  passed 240 C/JIT/GC cases, FD ownership, byte-array I/O/slices, 124 wire and 12
  generated-entry refusals, 2282 native/873 heap rollback refusals and controls.
- DIS64 compiler/image oracles: 67 scalar and 241 helper sources passed; both
  reported zero whole-console processes. These are not a full emulator boot.
- The concurrent host initially passed 71/71 plain and 72/72 TSan after the C7
  core refresh. Post-parser full plain replay passed 71/71; final-pin full TSan replay also passed 72/72; the final targeted host/owner
  subset passed 8/8. No reference VM execution was selected.

The fixed Sys gate and its dependency closure were also replayed directly from
our vendored tree, without a sibling Inferno checkout. New CI jobs build the
committed-parser DIS64 producer and run these source/image/Sys guards on native
AMD64 and ARM64. They do not select a single-threaded DisVM execution mode.

The active hosted NC64 uses magic 819264 and Word=8. The new eight-byte-signature
DIS64 uses its separately checked native layout; importing its headers/compiler
pilots does not authorize general file admission in our runtime. Public managed
kind IDs remain a bilateral contract; candidate schemas are not assigned IDs.
The fixed Sys trial does not establish dial/announce/listen, network RPC,
automatic remote-proxy GC or compiler-transparent proxies.

## Продолжение proxy-блока

[Рабочий контракт](remote/README.md) восстановлен из существующих документов,
а не придуман вместо прошлых обсуждений. Следующий код должен начинаться с
explicit owner/proxy/request/lease boundaries и сетевых fault tests. Синхронизация
не объявляет сами удалённые переменные и функции реализованными.
