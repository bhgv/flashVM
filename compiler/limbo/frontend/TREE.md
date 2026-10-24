# Owned Limbo grammar trees

The [authoritative Coco/R grammar](limbo.atg) now optionally records all 60
production kinds into an immutable tree. This is a concrete **production tree**:
terminals/operators are retained in the copied source, not emitted as separate
terminal nodes. It is not a typed AST, symbol table, semantic checker, Dis backend
or SWF writer. Existing syntax-only calls do not allocate tree nodes.

## C99 ownership contract

[frontend.h](frontend.h) declares the API; [tree.c](tree.c) owns the implementation.

```c
LimboTree *tree = NULL;
LimboDiagnostic diagnostic;
LimboNode root;
if (limbo_tree_parse(bytes, size, &tree, &diagnostic)) {
    if (limbo_tree_node(tree, 0, &root)) {
        /* Use root.kind and numeric links while retaining the tree owner. */
    }
    limbo_tree_destroy(tree);
}
```

`limbo_tree_parse` returns 1 only for accepted syntax and a complete owned tree.
On every failure `*out` is unchanged. On success it transfers a **new owner**:
the caller must retain/release any previous tree itself. The input is copied;
subsequent caller-buffer edits do not change nodes or source. Input, output and
diagnostic storage must be accessible and disjoint. No path is retained or opened.
`limbo_tree_destroy(NULL)` is safe; ordinary double destruction is not.

`limbo_tree_count`, `limbo_tree_size`, `limbo_tree_source`, `limbo_tree_node` and
`limbo_tree_locate` inspect immutable data, independently of parser globals.
Borrowed source lasts until destroy. Copied node links refer to indices in the
same tree, not another parse/module instance. `kind` names are static strings.
Null metadata owners return zero/null; invalid node/context queries preserve
caller output. Source is NUL-terminated, size excludes that terminator.

Parsing still uses the existing namespaced Coco/R globals and **must be externally
serialized**, including syntax-only calls. Read-only queries of retained trees
may overlap other parsing, but owners must synchronize destruction with readers.
No Dis heap pointers, channels, callbacks or GC roots are introduced here.

## Topology and byte spans

Nodes have preorder indices. Root is index 0, kind `Limbo`, and covers the complete
source snapshot. `SIZE_MAX` means absent parent/child/sibling. `first_child` plus
`next_sibling` enumerate direct children in authored parse order; parents precede
children. No internal array pointer is exposed across reallocations.

`[begin,end)` offsets count **UTF-8 bytes**, including BOM/CRLF bytes as authored;
they are not character indices, display columns or Limbo `.sbl` column values.
Empty productions anchor at the next lookahead token; ancestor ends include all
such anchors. Consequently a span can include intervening comments/whitespace
and should not be mistaken for an exact lexeme. Empty children remain represented
so optional production structure is observable. `limbo_tree_locate(offset)` finds
the deepest nonempty covering production (earliest preorder node on equal depth).
EOF is outside every half-open span. Leading/trailing trivia falls back to the
root or a containing production. Context lookup is O(nodes), constant extra space,
without recursion or allocation; it does not perform name resolution.

## Resource and failure boundaries

The existing 2 MiB UTF-8/NUL, 512 guarded nesting and 4000000 guarded-visit limits
remain. Tree recording adds **262144 nodes** maximum. Storage grows geometrically
with checked byte multiplication; source and partial arrays are released on error.
Diagnostic -5 is the tree node/storage bound, -2 allocation failure. Other syntax,
encoding and nesting/work diagnostics retain their previous meaning. If syntax
and recording both fail, the syntax diagnostic takes precedence. Syntax-only
acceptance retains its own budget even when the same source exceeds the tree bound.
Test-only allocation injection covers the owner, copied source and each node-array
growth allocation, with recovery and retention of an older successful tree.

## Native inspector

Build using the normal explicit external Coco/R paths, then run:

```sh
build/frontend/limbo_tree program.b
```

When configured directly from `frontend/`, the executable is `build/limbo_tree`.
It emits `source<TAB>bytes`, `nodes<TAB>count`, followed by rows:
`node, index, kind, begin, end, parent, first_child, next_sibling` (tab separated).
Absent indices print `-`. Exit 0 means a tree was emitted, 1 means rejected input
or output failure, 2 means usage/input I/O failure. Diagnostics go to stderr;
rejected input emits no partial tree. Original `limbo_parse` behavior is retained.

## Compiler and Dis direction

Use this owned parse product as input to later AST/name/type/import/signature
passes and typed Dis lowering. Debug/source information must follow authored
coordinates through lowering; byte offsets here cannot be silently reused as
`.sbl` positions. Existing [symbol prefix reader](../symbols/README.md) remains a
separate component. This series does not import Dis or change its JIT/debug ABI.

Dis experiment was rechecked at
[`72616af5`](https://github.com/bhgv/Inferno-OS-bhgv/commit/72616af5ccb7a89473596c2a1409c1f064bc9f6e).
Existing `appl/lib/debug.b` and `appl/wm/{deb,debdata,debsrc}.b` remain references
for debug semantics. No Inferno Draw/Tk/crypto dependency is added.

The planned compiler implementations remain C99 and Limbo, later AS, all through
Coco/R `.atg`. Reuse [coco-r-limbo](https://github.com/bhgv/coco-r-limbo).
The tested paths here are its **C-host/C-output** generator and bhgv/coco-r-C;
this does not validate native Limbo generator execution or implement the Limbo
compiler. Embedded actions in this grammar are C99 and need a Limbo implementation.
