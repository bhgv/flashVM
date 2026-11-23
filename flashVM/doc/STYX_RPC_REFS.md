# Ordinary Dis ref arguments in FlashVM

[Русский](STYX_RPC_REFS.ru.md) · [Managed import](STYX_RPC_IMPORT_MANAGED.md) · [Managed export](STYX_RPC_EXPORT_MANAGED.md)

The ordinary managed profile carries a first reference definition, aliases
to that definition, and a mask selecting changed original arguments. Both
directions use the existing libstyxrpc request and reply codec. For an ADT
with two fields, such as `text: string` and `bytes: array of byte`, the AS
value for `ref` is a dense native Array `[text, bytes]`. Field position is
compiled schema order. AS `null` maps to Dis nil. Passing the same Array
twice preserves identity inside the receiving call.

Use up to eight ref argument positions and up to eight direct fields in
each ref. A ref may appear among arguments and their tuple fields. Its
fields are copied by value. Keep the ref Array dense and at its original
length, with own data slots and no watchers or custom accessors. Do not
install readonly attributes on slots that must receive updates.

The server compares encoded fields to the original snapshots and emits
one update per changed definition. An AS import validates and decodes the
whole reply, including every update, before modifying any original
Array. It checks that the destination slots are still valid and that
changed slots are writable. It then writes all staged changes without
callbacks or allocation. Equal fields keep their current values, so an
unchanged array field keeps its identity. The result and updated values
remain rooted through the operation. A remote error or local commit
failure leaves all participating local ref slots intact. A local commit
failure can happen after the remote function has run.

Use the [positional graph binding](STYX_RPC_GRAPH.md) for returned refs,
refs nested within ADTs or arrays, and sparse graph deltas. The independent SDK
also supports live channels.

The [Limbo interface](https://github.com/bhgv/flashVM/blob/master/lib/styxrpc/tests/flashvm/refs.m) and
[FlashVM acceptance host](../tests/styxrpc/refs.c) exercise aliasing,
unchanged fields and rollback in both directions. The DisVM runner uses
both interpreter and JIT modes.
