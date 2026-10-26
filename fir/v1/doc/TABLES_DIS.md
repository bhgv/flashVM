# Our Limbo tables → FIR → native Dis

[Русский](TABLES_DIS.ru.md) · [Source/FIR contract](TABLES.md) ·
[Source example](../examples/tables/main.b) · [C99 compiler client](../examples/tables/compile.c)

`keys[i]` now returns a checked snapshot key, usable as `t[keys[i]]` for reads or writes.
Bounds failures remain eager even for unused keys. Keys retain their immutable snapshot
when locals or the original table are replaced; numeric/text namespaces stay distinct.
See [Extension status](EXTENSIONS.md).

FIR compiler 0.16 emits the supported single-block big-table subset directly as
NC64 Dis. It does not call the temporary Limbo compiler, an assembler, Python or
the reference execution library. The destination uses the existing HASLDT import
table and existing `load`, `mframe`, `mcall`, `lea`, scalar and pointer moves.

The resulting image requires the matching hosted `$Bigtable` module in runtime
0.19 or later with this ABI. Its imported function signatures are checked by the
real Dis loader. A missing helper or mismatched signature fails before the
generated operation can invoke another native function. This does not change
Inferno, the Dis container or the instruction set.

```sh
build/fir/fir1_source limbo --input fir/v1/examples/tables/main.b tables.fir -g tables.debug.json
build/fir/fir1 check tables.fir
build/fir/fir1 opt tables.fir tables.opt.fir
build/fir/fir1 dis tables.opt.fir tables.dis
build/runtime/disvm --no-jit --stats --inspect-words 0 2 tables.dis
build/runtime/disvm --jit --stats --inspect-words 0 2 tables.dis
```

Both executions capture `DATA 0 42` and `DATA 8 1`. The second cell is the executed
flag, not a second source result. The independent C reference runner is still
available for comparison. SWO/SWF table lowering remains unsupported.

## ABI and ownership

All offsets are explicitly NC64 eight-byte cells, independent of host C pointer
sizes. Scalar big values use exact MOVL payloads. A table or key snapshot is one
rooted owner reference. A scalar-key table location occupies two cells: retained owner and
captured numeric/string key. Only string keys make the second cell a heap root.
A range occupies two scalar cells. A snapshot key occupies two cells (rooted snapshot and scalar position); a location using it occupies three cells (table owner, snapshot owner, position). Typed copies preserve each exact layout.

The entry-frame descriptor marks loaded module instances, strings, tables,
snapshots and location owners/keys. Numeric indices, ranges, status/value pairs
and raw prepared stack-frame pointers are not marked as heap objects. Module data
has a separate map for the import path and copied constant strings; scalar trace
captures and numeric constants remain unmapped. Native callee frames use the
loader's checked builtin descriptors and retaining MOVP argument transfers.

Read/write status is checked by an explicit native helper. A missing key cannot
silently become zero. Numeric location creation and range construction validate
their operands eagerly, including unused results preserved by the optimizer.
Native exception diagnostics resolve the bytecode caller PC, avoiding pointer
subtraction against a builtin's absent program segment.

The API borrows the module and output stream. Coverage/layout/staging failures
write no output. A final output I/O failure may leave a prefix; the CLI stages
before replacing an existing destination. Supported operations are deliberately
single-block; table CFG/loops, general language modules,
any and AS/SWF adapters remain pending. FIRDBG1 source records remain
separate; full source-to-Dis SBL/PC mapping is not claimed.

## Collector and inspection acceptance

Use `--quantum 1` to schedule after each instruction and exercise collection
between frame preparation, argument moves and native calls. The normal quantum
remains 2048; the explicit option accepts 1..65536 and rejects duplicates.
`--stats` reports instruction/native counts, collector passes/epochs and remaining
native table/key owners. Successful table programs retire both owner counts to
zero after their entry frame exits.

Scalar inspection checks every requested word against the actual type map.
It accepts a plain numeric capture range even when other module cells contain
strings, but rejects mapped words and ranges crossing into them. A registered
native GC root retains the inspected module graph until report/error cleanup;
no heap address is exposed by these scalar views.

Acceptance runs original/optimized programs in interpreter and JIT modes, with
Unicode and distinct numeric/string keys, exact signed-big limits, sparse indices,
aliasing, copied locations/ranges/snapshots, large maps beyond inline prefixes,
bad import signatures, rejected pointer inspection and bounded caller-PC errors.
Native dispatch counters distinguish JIT leaves from interpreter execution; the
C table functions remain ordinary compiled runtime services.

[Reflection and standard Limbo big/string cast syntax](TYPE_REFLECTION.md).

[Exact big arithmetic and unary precedence](BIG_ARITHMETIC.md).
