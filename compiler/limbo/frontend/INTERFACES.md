# Explicit Limbo interface catalogs

Version 0.6.6. This C99 pass consumes the [existing Coco/R production tree](TREE.md),
not another handwritten language parser. [interface.h](interface.h) is the public
contract, [interface.c](interface.c) the bounded owner/index implementation, and
[interface_main.c](interface_main.c) the native inspector. It is the declaration
stage for a future `.m` → native-C backend, **not that backend itself**.

## Catalog and ownership

`limbo_interface_parse(bytes,size,&out,&diagnostic)` creates an owned source tree,
names and source-order declarations. Return 1 transfers a new owner; failures
preserve `*out`. Caller input may be overwritten afterwards. The caller retains
and releases any earlier owner itself. Release with `limbo_interface_destroy`;
null is safe. Input/output/diagnostic storage must be accessible and disjoint.
Parsing must be externally serialized with other tree/syntax parser calls.

`limbo_interface_tree` borrows the complete tree; never destroy it separately.
`limbo_interface_count` reports expanded declaration count.
`limbo_interface_declaration` copies a small view; name/source pointers remain
borrowed until catalog destruction. Invalid arguments/index preserve output.
`limbo_interface_find(scope,name,&index)` performs immutable exact-name binary
search. A module declaration index is its scope identity; `SIZE_MAX` means file
scope. A nonmodule/foreign/stale index must not be passed as a scope. Out-of-range
and nonmodule scopes return zero without changing output. Numeric indices from
another catalog cannot in general be detected: keep all identities owner-local.
Names compare exact UTF-8 bytes, case-sensitive, without Unicode normalization.
No parser globals, allocations, source paths or Dis objects are used by queries.
Synchronize catalog destruction with readers.

Each declaration records kind, copied name, scope identity, Top node index and
half-open name/type byte ranges in the copied source. Categories are module,
ADT, direct `fn`, constant, alias, exception, import, value and include. They are
**syntactic categories**: a value whose named type aliases a function remains
`value` until later type resolution. Type ranges retain the authored DeclType,
including annotations and any trivia/empty-rule anchors from the production tree.
They are not runtime signatures or exact normalized C types. ADT fields/methods,
pick variants, formal parameters and function returns remain in that complete
tree; this catalog does not separately index or semantically validate them.

Grouped names expand left to right and share the declaration's type range.
Nested modules have separate identities; identical export names in different
modules are legal. Same-scope repeated names are rejected before publication,
with the earliest repeated declaration's byte offset. Original source ordering
is preserved; a separate sorted index supports lookup. Field duplicates inside
an opaque ADT, imported duplicates and names from unexpanded includes are not
checked by this first pass.

## Explicit interface boundary

Includes are recorded with empty name and their complete Top span, never opened
or expanded. Unknown type names are retained, not resolved. This is not complete
Limbo semantic acceptance. Function definitions, inferred/tuple declarations,
`implement`, grouped module definitions and module scopes nested through an
unindexed ADT member are rejected as unsupported (-6). Grouped modules would
need scope/body expansion rather than silently attaching exports to one name.
Nested ordinary modules and grouped nonmodule declarations are supported.

Duplicate declarations report -7; over 16384 catalog entries/storage overflow
report -8; allocation failure reports -2. Existing syntax/tree/input bounds still
apply. Catalog diagnostics set byte offset; line/column are currently zero.
No partial catalog is published on any error. Empty files/comments yield an empty
catalog. Allocation recovery tests fail each owner, scope-index, name, array-growth
and sorted-key allocation while retaining an earlier successful catalog.

## CLI and build

The existing explicit external Coco/R build also creates `limbo_interfaces` and
`limbo_interface`. No generator runtime is vendored. For a root Limbo build:

```sh
build/frontend/limbo_interface module.m
```

Output starts `source<TAB>bytes`, `declarations<TAB>count`; each remaining tab-separated
row has fields `decl, index, kind, name, scope, node, name_begin, name_end,
type_begin, type_end`. File scope prints `-`. Names cannot contain tabs/newlines
under the current grammar. Exit 0 means catalog accepted/emitted, 1 means rejected
input/catalog or output failure, 2 means usage/input I/O error. Rejections emit
no partial stdout. The inspector does not emit or compile C.

Optional real-header acceptance reads only an explicitly selected checkout:

```sh
python3 tests/interface_upstream.py --upstream /path/to/Inferno-OS-bhgv \
  --tool /path/to/build/frontend/limbo_interface
```

Or configure `-DLIMBO_INTERFACE_UPSTREAM=/path/to/Inferno-OS-bhgv` for an extra
CTest. The test reads `.m` files and Sys/Math native table names; it does not
expand their includes, link their implementations or import UI/crypto libraries.

## Native C backend reference

Rechecked upstream experiment at
[`0a0a007e`](https://github.com/bhgv/Inferno-OS-bhgv/commit/0a0a007ebf6c0276ada93367dde149df06efce12).
Use the existing [C stubs generator](https://github.com/bhgv/Inferno-OS-bhgv/blob/0a0a007ebf6c0276ada93367dde149df06efce12/limbo/stubs.c)
and [Limbo stubs generator](https://github.com/bhgv/Inferno-OS-bhgv/blob/0a0a007ebf6c0276ada93367dde149df06efce12/appl/cmd/limbo/stubs.b)
as references, together with their type/signature/layout passes. In the existing
C compiler, `-a` emits ADT/frame declarations, `-t Module` native tables,
`-T Module` static C initialization/function skeletons; `-A`/`-d Module` select
dynamic variants. These are **upstream flags**, not options of our inspector.

Future native emission must preserve ADT layout/padding, frame return/argument
positions, GC pointer maps, signature generation, registration and dynamic
initialization. Do not infer pointer size from Limbo scalar WORD size or reuse
one host's generated offsets for another target. Type/import/name binding and an
explicit matching Dis ABI profile are prerequisites. Current acceptance matches
111 Sys/Math exported names only: no hash, frame, GC map, boot or JIT ABI claim.

The planned C99 and Limbo compiler implementations continue to use Coco/R `.atg`,
reusing [coco-r-limbo](https://github.com/bhgv/coco-r-limbo). This C pass is not a
Limbo compiler implementation or native Limbo-generator acceptance. Dis remains
unimported; no Inferno Draw/Tk/crypto dependency is added.
