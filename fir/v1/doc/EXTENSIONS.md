# Limbo extensions: executable scope

[Русский](EXTENSIONS.ru.md) · [Table source](TABLES.md) ·
[Dis ABI](TABLES_DIS.md) · [IDE projection interface](../../../ide/dis/bridge/ideprojection.m)

This describes the current implementation, not just accepted syntax or the target
architecture. The source compiler is our Coco/R frontend; the native bootstrap
remains a separate temporary tool for the ordinary Limbo IDE modules.

| Feature | Source → typed FIR → native Dis | Current boundary |
| --- | --- | --- |
| `table of big` | Implemented | Straight-line `value(): big` profile |
| Numeric/string keys and `t.k` | Implemented | Distinct key namespaces; big values |
| `len`, explicit slices, `keysof` | Implemented | Independent copied snapshots/slices |
| `keys[i]`, `t[keys[i]]` reads/writes | Implemented | Checked immutable snapshot key |
| `typeof`, heterogeneous tables | Pending | No fabricated static answer for dynamic data |
| Language `any` | Pending | Existing FIR `T_ANY` enum does not establish an implementation |
| Transparent proxy locals/operators | Pending | IDE uses explicit typed request/reply APIs |
| Object/method projections | Explicit IDE APIs | Owner identity/epoch descriptors; no native heap pointers |
| Network-wide automatic remote references | Pending | No general export/lease/GC registry is claimed |

## Snapshot keys

```limbo
# A complete source module is in examples/tables/main.b.
# keysof copies keys, not table values; the result has an inferred snapshot type.
t := table of big;
t["1"] = big 42;  # String namespace, independent of numeric 1.
keys := keysof t;
k := keys[0];     # Check bounds now, even if k is never read later.
keys = keysof table of big; # k still retains the old immutable snapshot.
t[k] = big 43;    # Resolve the original string key; copy new text only on insertion.
return t[k];      # Read the current table value, not a value frozen by keysof.
```

FIR appends `table.key` and `keys.at` without renumbering existing enum IDs.
A key is a snapshot-owner/position capability. Its internal two-cell native
representation never crosses a channel or network. A table location using it
retains both the table and snapshot in three cells; only the position is scalar.
The compiler emits precise GC maps and signature-checked `keycheck/getkey/setkey`
calls. Bounds, missing keys and allocation failures stay observable through
optimization. Snapshot-key indexing allocates no additional key/String owner.

The C reference executor retains snapshots until invocation teardown; Dis uses
actual frame roots, RC and GC. Tests replace source owners under per-instruction
collection and require zero live native owners after successful frame retirement.
The scalar C core holds copied key bytes and big values, never Dis/Flash pointers.

## Proxy and any boundary

[Ideprojection](../../../ide/dis/bridge/ideprojection.m) and
[Idevalue](../../../ide/dis/bridge/idevalue.m) already support explicit typed
reads, writes, copied conversions, object/method identity and owner invocation.
These ordinary Limbo modules use request/reply channels. They are not the compiler
semantics of an automatic `any` local. A shallow IDE value snapshot is also not
this native mutable `table of big` owner.

The agreed future `any` must adapt typed channel access: read or convert only
when the consuming operation requires a type, write through the owner, and invoke
functions in their owning environment. That requires FIR capability/effect types,
source-occurrence diagnostics, owner incarnation and lifecycle handling before
transparent syntax is enabled. Atomic compound updates must execute at the owner;
a client read followed by a client write is not an atomic implementation.
No table-language extension from this series is exported to Inferno.
