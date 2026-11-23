# Styx managed table records

[Русский](MANAGED-TABLE-RECORD.ru.md) · [Declaration](call_record.h) · [Implementation](managed_table_wire.inc) · [Two-VM example](../../examples/remote-scalar/managed-fid/README.md#pass-hybrid-tables)

The same native format and table algorithms are available in the [C SDK](../../../lib/styxrpc/reference/hybrid_tables.md) and [FlashVM graph binding](../../../flashVM/doc/STYX_RPC_HYBRID.md).

The managed call profile carries the native `Bigtable->Handle` as shape `h`.
A first occurrence uses graph tag 10, the one-byte object index and a complete
table record. Later references use the same index. A returned reference to an
argument keeps the original caller handle. A new result gets a local native
table. The compiled shape and the positional graph profile are required on
both ends of the call.

| S-record | Tag | Payload |
| --- | --- | --- |
| Snapshot | 20 | Zero or more complete pair records |
| Pair | 21 | One key record followed by one signed `big` value record |
| Patch | 22 | Removals followed by changed or new pairs |
| Removal | 23 | One key record |

A key is a nonnegative `big` (tag 2), or counted UTF-8 text (tag 7).
Numeric keys are ascending in a snapshot, including both the dense prefix
and sparse indices. Text keys follow in insertion order and may contain
zero bytes. A patch is associated with an original argument position or
a nested object index by the enclosing graph update. Applying a patch can
move a text key by removing and reinserting it. An unchanged table produces
no update record.

## styxmanaged_table_visit

Include [call_record.h](call_record.h) and link the C Styx terminal library.

```c
int styxmanaged_table_visit(const void *bytes, size_t size, int patch,
    int (*visit)(void *, const StyxTableKey *, int, int64_t), void *state);
```

`bytes` and `size` identify one complete table S-record in borrowed storage.
`patch` selects a snapshot with value zero or a patch with a nonzero value.
The function checks tags, lengths, key encoding and record order. It returns
one on success or zero on invalid input or visitor refusal. The managed
runtime also checks text-key uniqueness when constructing a snapshot.

Pass a null visitor to check the wire format. Otherwise `visit` receives the
borrowed `state`, the decoded key, a nonzero removal flag or zero for a pair,
and the pair's signed value. The value is zero for a removal. The key and its
text are borrowed for the callback. The codec performs no I/O. A late parse
error may follow earlier callbacks, so the visitor must stage changes in
private storage and publish them only on success.

```c
int complete = styxmanaged_table_visit(record, record_size, 0, NULL, NULL);
```

See [reference updates in the call profile](SCALAR-FUNCTION.md) and the
[wire checks](tests/scalar/table_records.inc).
