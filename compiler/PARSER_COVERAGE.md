[Contextual asinc checkpoint: syntax only, executable lowering pending](ASINC.md) · [RU](ASINC.ru.md)

# Parser coverage, series 0.6.0

Specific lexical gaps and ambiguities are closed. Full conformance to every version of all five languages is not established; syntax recognition is separate from AST construction, semantics and object generation.

[Detailed lexical profile](LEXICAL_PROFILE.md)

| Parser | Version | Current coverage |
| --- | --- | --- |
| AS1 | 0.4.0 | Expressions/statements, legacy operators, ASI, authoring wrappers, Unicode/escapes |
| AS2 | 0.4.0 | AS1 syntax plus classes/interfaces/types, Unicode/escapes |
| AS3 | 0.5.0 | Packages/classes/interfaces, namespaces, metadata, ASI, E4X, Vector literals, CONFIG blocks, Unicode/escapes, structural regexp validation |
| Limbo | 0.4.2 | Modules/ADT/polymorphism, fixed, exceptions, channels, casts, slices, exports and brace-containing qualifiers |
| FIR syntax | 0.1.1 | FIR 1.0 modules/declarations, composite types, functions, SSA/block/edge parameters, invoke/debug records |

ATGs remain authoritative and human-formatted. Embeddable C99 APIs are
`as1_check/read`, `as2_check/read`, `as3_check/read`, `limbo_check/read` and
`firsyn_check/read`. They do not print/exit; input ownership stays with the host.
Serialize calls to one parser; no reentrancy/thread-safety promise is made.
CLIs are separate adapters. Limits are 2 MiB input, nesting 512 and 4,000,000
visits to guarded productions; regexp recursion uses the same guard.

FIR tools are 0.5.1. Executable FIR remains a separately verified subset:
`fir1 syntax` never bypasses verifier/codegen. Object generation emits SWO;
the linker emits SWF. Extended structural FIR does not automatically become
executable IR. The player version is unchanged.

Remaining work includes ASTs, types/context/lvalues, complete regexp-engine
compatibility, XML namespace/entity resolution, include expansion and
extended FIR verification/lowering. The Unicode profile is pinned to 3.2.0
with documented additions, not claimed identical to every Flash release.

Linux x86-64 validation: 27 compiler CTest groups, 7 FIR groups, the separate
five-C-API host, and 27 compiler groups with UBSan. Detached renamed copies
also built and passed. ARM64 Linux/musl, RISC-V64 Linux/musl and Windows
x86-64/GNU cross-builds were repeated; ELF/PE machine IDs were checked,
without foreign execution. These used host-adapted Zig 0.13.
Coco/R b91b652 plus the separate errno fix was used; external source is not
included in FlashVM. No new ASan, Android/macOS execution or complete
language-conformance suite is claimed.

FlashVM baseline: 39eb951c867382d27eb05dd151e3ed2243e50c28 (master 0.5.2).
Earlier [VALIDATION](VALIDATION.md) reports retain their recorded versions.

## Build / Сборка

```sh
cmake -S compiler -B build-five \
  -DCOCOR_ROOT=/host/coco-r-C \
  -DCOCOR_EXECUTABLE=/host/coco-r-C/sources/cocor \
  -DFIR_SYNTAX_ROOT=/absolute/path/to/fir/v1/frontend
cmake --build build-five
ctest --test-dir build-five --output-on-failure
```

[Native/cross build guide](../BUILDING.md) ·
[Five-API C host](tests/embed_five.c) · [Examples](examples.md) ·
[AS1 ATG](as1/frontend/as1.atg) · [AS2 ATG](as2/frontend/as2.atg) ·
[AS3 ATG](as3/frontend/as3.atg) · [Limbo ATG](limbo/frontend/limbo.atg) ·
[FIR ATG](../fir/v1/frontend/firsyn.atg).
