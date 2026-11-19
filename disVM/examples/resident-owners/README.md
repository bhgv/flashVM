# Permanent native Dis owners

[Русский](README.ru.md) · [Source](main.b) · [Menu](../../../ide/ENTRYPOINTS.md)

This console example starts four Dis children. Each child keeps its own native
thread, private scratch array and error state, then sends a copied `(lane, sum)`
reply. The parent verifies all four results. Returning from the entry lets the
runtime join owners normally. No display/FlashVM context is needed; ordinary
inherited standard descriptors remain available.

Run installed menu **19**, or:

```sh
sh out/disvm-editor/run-resident-owners.sh --jit
sh out/disvm-editor/run-resident-owners.sh --no-jit
```

The same launcher is included in `disvm-runtime`. Arguments are literal runtime
options: `--jit` selects JIT dispatch; `--no-jit` selects interpretation. The
launcher selects the required native resident model and prints statistics. There
is no worker-count setting or process migration. The obsolete
`--managed-workers` option is rejected by the VM; the installer removes its old
launcher and bytecode from an existing package.

Expected application output:

```text
PASS resident owners: children=4 sums=ok private-state=ok channels=ok
```

Read [`calculate`](main.b) first: `lane` selects one of four sums and `replies`
remains borrowed through its one send. Scratch storage is private; only two scalar
values cross the channel. [`init`](main.b) roots the reply channel, starts the
children and validates each lane once. Any failed check raises an error.

The [acceptance check](../../runtime/test_resident_owners.py) independently checks
five native owners (parent plus four children), joined cleanup, no migrated active
owner, an empty in-flight queue and the requested JIT/interpreter counters. It does
not measure CPU scaling or demonstrate live thread migration.
