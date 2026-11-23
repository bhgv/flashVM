# Dis hybrid tables in independent C programs

[Русский](hybrid_tables.ru.md) · [SDK](../README.md) · [Graph client](graph_invoke.md) · [Graph server](graph_module_init.md)

Include `styxrpc/hybrid.h` and link the portable `StyxRpc::rpc` target. Each application owns its `StyxRpcHybrid` tables and serializes access on its owner. The SDK and DisVM share the C table storage and the ordered snapshot and patch algorithms. You can use the table without a VM or a hosted Styx connection.

1. [Open](hybrid_open.md) and [edit](hybrid_apply.md) a table. Numeric indices and counted UTF-8 keys are separate. Values use signed 64-bit integers.
2. [Write an ordered snapshot](hybrid_snapshot.md). In a graph profile, use shape `h`. A first graph ref carries its one-byte ID followed by the snapshot. Later references carry the ID.
3. When building a reply, use [graph diff](graph_diff.md) with full original and updated definitions to wrap the [ordered key patch](hybrid_diff.md) under the correct positional or nested selector.
4. At the caller, resolve the original table with [graph update index](graph_update_index.md). The update record's first payload byte is the selector. The remaining bytes are the complete table patch. [Stage it](hybrid_stage.md) against current contents and verify all remaining changes from the reply. Publish the replacement locally only on success. [Close](hybrid_close.md) each superseded or discarded table.

[Decimal parse](hybrid_big_parse.md) and [format](hybrid_big_format.md) preserve the entire signed 64-bit range. The [portable C fixture](../tests/hybrid.c) exercises graph snapshots, staging, malformed patches and exact boundary values. For a live reverse VFS table instead of a graph snapshot, see [`styxrpc_program_bigtable`](program_bigtable.md).
