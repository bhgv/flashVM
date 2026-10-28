# Outer alias resolution — Coco/R Limbo

[Русский](ALIASES.ru.md) · [API](resolve.h) · [Implementation](resolve.c) ·
[Authoritative grammar](limbo.atg) · [Semantic tests](tests/resolve.c) ·
[Allocation recovery](tests/resolve_alloc.c) · [Native C profile](NATIVE_C.md)

The interface catalog now has an explicit immutable binding pass:
`limbo_interface_declared_type` obtains a direct Type production from an alias,
function or value declaration; `limbo_interface_resolve_type` follows outer aliases
and returns the final Type node. Both operate on an existing catalog and can be
used through the C99 `limbo_interfaces` library independently of the CLI.

NamedType now contains exact `TypeIdentifier` token-span nodes produced by Coco/R.
The resolver uses those nodes and catalog lookup. It does not tokenize source
again, evaluate expressions or load files. Comments around a type name and UTF-8
identifiers therefore retain the parser's authoritative boundaries.

## Lexical rules and lifetime

An unqualified name is sought in the current module, its enclosing modules, then file scope. A found
value/function is an error rather than permission to fall through to a different
file alias. Once an alias is followed, resolution continues in **that alias's
own declaration scope**. A module shadow must not rebind a file alias's body.
Forward references within the accepted interface are allowed. The native C profile
still requires exactly one top-level module; nested-module resolution is a
library query capability, not new nested-module native emission.

For example, file-scope `A: type big; B: type A;` plus module-local
`A: type real;` means a module use of B remains big, while a use of A is real.
Cycles are detected by declaration identity. At most `LIMBO_ALIAS_MAX_DEPTH`
(256) distinct edges are followed iteratively, without recursive C calls.

Outputs change only on success. Diagnostic -1 identifies an invalid query, -2 a
temporary name allocation failure, -10 missing/non-alias or unsupported qualified/
generic binding, -11 a cycle or depth limit. The diagnostic points to the failing
source token/construct using LF-based lines and UTF-8 **byte** columns/offsets;
these must be translated before showing rune-based IDE caret positions.

The catalog owns all tree/source data. Returned indices are borrowed from it;
synchronize destruction. Queries do not use parser globals or retain names and
may read immutable catalogs independently. Input/output/diagnostic storage must
be disjoint. The underlying Coco/R parser remains serialized and non-reentrant.

## Exact scope of this pass

This resolves **outer aliases only**. An alias to `array of big` returns that
array Type node; aliases nested inside array/ref/function/tuple members are not
traversed. Qualified module/ADT names, generic instantiations and includes/imports
are not resolved. This is not a complete typed AST, body type checker or FIR
lowerer. Existing JSON syntax views still report named source types.

The native C profile accepts only direct byte/int/big/real/string or aliases that
end at those primitives. It validates even unused alias declarations, then emits
canonical WORD/LONG/REAL/BYTE/String pointer frames, signatures and GC maps.
It emits no C typedef for a source alias; its spelling cannot alter ABI layout.
Composite aliases remain rejected by that backend before output opens.

## Executed checks

Semantic tests cover forward chains, lexical shadows, root visibility, comments,
Unicode names, composite outer results, cycles, missing/qualified/generic names,
exact diagnostic byte positions, output retention and the 256/257 edge boundary.
A faulted resolver allocation leaves the same catalog reusable.
Native ABI tests compare alias and primitive metadata for both classic32/NC64
against independent MD5/layout calculations and explicit Inferno compiler oracles.
The [native module tutorial](../../../disVM/examples/native-module/README.md)
uses real aliases and runs calls, string GC, errors and typed channels on DisVM.
Its generated header is byte-identical to the former direct-primitive interface.
