# Lexical terminal arms — compiler 0.22

[Русский](TERMINAL_ARMS.ru.md) · [Commented example](../examples/tables/arms.b) ·
[Examples](../examples/tables/README.md) · [Table operations](TABLE_MUTATION.md)

The authoritative Coco/R Limbo grammar admits declarations, assignments and
ordinary profile statements inside both arms of a terminal decision. This works
for homogeneous `int` and exact `big`/table result profiles. Every arm ends in
`return` or another complete terminal `if/else`. There is no continuation after
the tree, no SSA phi, source loop or general function-call implementation.

```limbo
x := big 41;
if (document.flag == 1) {
    x = big 100;             # Assign the outer binding only on this runtime path.
    local := table of big;  # This owner belongs to the true arm's lexical scope.
    local.answer = x;
    return local.answer;
} else {
    return x + 1;           # This arm still sees the incoming x = 41.
}
```

## Initializers, types and scopes

`x := x + 1` inside an arm resolves the initializer against the incoming outer
binding, then creates a local shadow. A shadow may have a different supported
static type: `x := "42"` can shadow an outer big; `return big x` converts it
explicitly. An ordinary `x = ...` keeps the outer type and rejects a string
assignment to big. Duplicate declarations in the same scope fail. Explicit
uninitialized declarations remain unreadable until initialized. Sibling-local
names never become visible to the other arm, even if that arm cannot execute.

`int` arms retain ordinary homogeneous numeric statement rules. Table arms admit
existing typed local bindings, owner/key snapshots, table writes/deletion and
conversions. The compiler still type-checks both arms before optimization.

The frontend snapshots the incoming SSA binding prefix at each decision. Before
parsing the sibling, it restores this prefix and retires the first arm's names.
Nested snapshots are independent. SSA serials and local identities are never
reused across arms. The snapshot contains compiler identities, not runtime table
contents: only the selected arm performs its actual memory effects. There is no
implicit transaction, mutex or remote read/modify/write protocol.

No FIR operation, Dis opcode, Bigtable import or file-container change is needed.
Existing CFG verification checks dominance; effects and checked imports are kept
through optimization. Each arm still returns independently. Runtime owner maps
keep local table/key references alive across channel calls and one-instruction
scheduler/GC slices. At module termination no table/key owner remains live.

## Debugging and limits

Source sidecars include balanced `scope-enter`/`scope-leave` events at exact brace
byte spans, distinct scope identities, lexical depths and binding identities.
Reads and assignments to an outer name retain that name's declaration owner;
shadow declarations get a new owner. UTF-8 source byte offsets remain exact.
This does not add full Dis SBL or full debugger attachment.

The existing limits remain: shared syntax nesting 64, 127 decisions/255 blocks,
1000 source values and 256 total binding identities. Retired sibling locals count
toward the binding limit. Syntax/type/resource failures preserve prior FIR and
source-sidecar destinations. There is no implicit fallback to a bootstrap parser.

## Build and run

From the repository root after building FIR and runtime:

```sh
mkdir -p build/tmp
export TMPDIR="$PWD/build/tmp"
build/fir/fir1_source limbo --input fir/v1/examples/tables/arms.b build/arms.fir -g build/arms.json
build/fir/fir1 check build/arms.fir
build/fir/fir1 opt build/arms.fir build/arms-opt.fir
build/fir/fir1 dis build/arms.fir build/arms.dis
build/runtime/disvm --jit --quantum 1 --inspect-words 0 4 build/arms.dis
build/runtime/disvm --no-jit --quantum 1 --inspect-words 0 4 build/arms.dis
```

The two value cells are `42, 0`; their executed flags are `1, 0`. Both original
and optimized tutorials are compiled by relocated C99 SDK clients and executed
in both VM modes. CLI and library APIs share these grammar and semantic actions.
Scalar int arm programs also pass SWF object emission; this series does not claim
physical ARM64 execution or a new complete language implementation.
