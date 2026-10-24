# Limbo tree acceptance, 0.5.6

Checked on 2026-10-24. The component is C99, with extensions disabled and an
explicit external generator. No compiler/runtime/generator sources are vendored.

- External coco-r-limbo C-host/C-output: all eight CTest targets pass.
- External coco-r-C: all eight CTest targets pass.
- ASan+UBSan: all eight targets pass; tree ownership/structure/allocation/context
  targets rerun after adding guarded-nesting recovery and pass. Leak detection
  is disabled; no thread-race or scanner allocation-fault acceptance is claimed.
- Detached/renamed copy of compiler/limbo plus LICENSE: builds independently and
  all eight CTest targets pass with the external coco-r-limbo host generator.
- Shared syntax/tree corpus: 64 accepted/rejected cases; successful trees cover
  all 60 production kinds. Numeric relationships, complete reachability,
  parent/sibling ordering and containment are checked independently from storage.
- Expression fragments verify additive/product/power precedence; byte-context
  lookup is compared against a parent-link depth oracle at every byte of six
  LF/CRLF/BOM/comment/UTF-8/empty/nested fixtures.
- C ownership tests retain a source tree across input overwrite, 32 malformed/
  valid parse cycles, nesting exhaustion, tree exhaustion and later recovery.
  Error outputs and metadata ownership remain unchanged; syntax-only parsing
  still accepts the flat source that exceeds the tree node budget.
- Allocation injection fails each observed tree owner/source/array growth
  allocation exactly once, retaining an old owner and testing subsequent recovery.
- Coco/R grammar formatter reports zero noncanonical files. Generated parser
  and scanner artifacts remain in build directories.

The previous predicate reproducibly rejects `x!=nil` as a named unary cast at
byte 19 in `f(){ x:="中"; if(x!=nil) { return; } }`. The fixed predicate treats
contiguous != as comparison. Added regressions include qualified/indexed operands,
comments, invalid missing operands/split tokens, and retained named !/~ casts.

This acceptance establishes an owned concrete production tree and source context
API. It does not establish typed AST semantics, import/type/signature checks,
Limbo implementation/native Limbo generator execution, Dis/SWF code generation,
Dis embedding/debug control, JIT changes or real-device execution. The actively
maintained Dis experiment branch was rechecked at 72616af5; no upstream VM files
are imported or modified by this series.
