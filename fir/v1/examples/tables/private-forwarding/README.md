# Private numeric table forwarding

Run from the repository root after building FIR with the optional table core:

```sh
export TMPDIR="${TMPDIR:-$PWD/build/tmp}"
mkdir -p "$TMPDIR"
build/fir1/fir1 opt fir/v1/examples/tables/private-forwarding/main.fir /path/to/optimized.fir
build/fir1/fir1_table_run fir/v1/examples/tables/private-forwarding/main.fir
build/fir1/fir1_table_run /path/to/optimized.fir
```

Both executions print `42`. Choose an existing writable output directory.

The module allocates a fresh numeric table. `key` is absolute zero, and `value`
is the immutable integer 42. `table.index` captures that table/key pair;
`table.store` inserts the value. `table.lock` retains the explicit table entry.
The following load can reuse the stored SSA value because the table has remained
private, the key is identical, and no mutation has invalidated the candidate.
`table.unlock` releases the retained entry before tracing the result. The trace
is an opaque boundary; nothing after it inherits the earlier privacy proof.

Inspect the optimized file: the load disappears, while allocation, store and both
lease operations stay. Removing the explicit lease makes this example ineligible
for forwarding. Changing the key or inserting a deletion also prevents it.
The language does not accept comments everywhere; this companion text explains
every instruction without introducing unsupported syntax into the executable
example. See [the proof contract](../../../doc/LOCALITY.md) for all exclusions.
