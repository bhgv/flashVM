# Hybrid tables over Styx RPC

[Русский](STYX_RPC_HYBRID.ru.md) · [Positional graphs](STYX_RPC_GRAPH.md) · [Independent C SDK](../../lib/styxrpc/reference/hybrid_tables.md)

A native Limbo `ref Bigtable->Handle` is the `h` token in the Dis graph profile. It can appear in an argument, result, array or ADT. The first reference carries an ordered snapshot and subsequent references carry its graph ID. When one table is sent several times or returned to its caller, the receiving owner uses the same local object for each alias. A table newly created by an export becomes a new local object at the caller.

Install the [constructor](STYX_RPC_HYBRID_BIND.md) on the FlashVM owner. Call it without arguments and use the resulting table with the existing graph [import](STYX_RPC_IMPORT_GRAPH.md) or [export](STYX_RPC_EXPORT_GRAPH.md). The compiled Limbo module continues to use its ordinary Bigtable operations.

| Method | Arguments | Result |
| --- | --- | --- |
| `getIndex` | Exact numeric index | Stored value or `undefined` if absent |
| `setIndex` | Exact numeric index, signed `big` | Boolean success |
| `deleteIndex` | Exact numeric index | Boolean success |
| `getText` | Counted UTF-8 key | Stored value or `undefined` if absent |
| `setText` | Counted UTF-8 key, signed `big` | Boolean success |
| `deleteText` | Counted UTF-8 key | Boolean success |
| `length` | None | Number of contiguous entries from index zero |
| `count` | None | Total number of entries |

Signed integers within ±(2⁵³−1) can use AS `Number`. Use exact decimal strings in the index methods or as values to cover the full signed 64-bit range. Gets return an exact `Number` within that same bound or a decimal string outside it. A negative index is relative to the current dense length. A delete creates a hole without shifting later keys. String keys are separate from numeric keys and can contain U+0000.

The owner sends numeric keys in ascending order and text keys in insertion order. A returned patch removes keys before applying writes. It stages against current caller contents, preserving unrelated keys. It is committed with ordinary ADT changes only after all destination slots are checked. An error leaves the caller's table contents and ref slots intact. A local rejection can occur after the provider has executed.

The table object and its methods belong to one FlashVM context, which owns the underlying C storage through GC. The mounted graph transport uses its existing 64 KiB frame limit. Live table projections in a reverse directory are a separate SDK operation. ActionScript reverse-resource declaration and live channel bindings are open.

[Limbo interface](../../lib/styxrpc/tests/flashvm/hybrid.m) · [Dis client](../../lib/styxrpc/tests/flashvm/hybrid_client.b) · [FlashVM AVM1 implementation and caller checks](../tests/styxrpc/hybrid.c)
