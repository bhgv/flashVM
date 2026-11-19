# libstyx: from the first request to your service

Copyright (c) 2026 bhgv

[README](../README.md) · [API](index.md) · [Introduction](introduction.md) · [Examples](../examples/README.md)

1. Build the hosted configuration from README. Run `styx-flat 0`; stdout reports the actual port. Read `status` using `styx-cat PORT status`; expect `ready`.
2. Read [catalog.h](../examples/catalog.h). CatalogRow describes one object. catalog_query receives operation, ID, starting position, output array, capacity and result count. Root ID is 0; native pointers are not wire identities.
3. Read [fixed.c](../examples/fixed.c). Stat returns the selected object; list returns one immediate-child page. Keep identities stable independently of names.
4. Read [server.c](../examples/server.c): walk_node resolves a component, open_node validates mode, read_node distinguishes file/directory reads, attributes fills Dir, and close_node releases the pin. Dir strings must survive encoding. Report success only after successful backend work.
5. Separate hiding a name from destroying an opened object. [live.c](../examples/live.c) uses catalog_pin/catalog_unpin. Test opening title, hiding its name through ctl, reading the original fid and finally Tclunk.
6. Advance to database and HTTP examples. Queries fetch live pages rather than freezing the tree at mount. Directory offsets count encoded bytes, not children; return complete Dir records only. Small buffers and disappeared objects are errors, not automatic EOF.
7. Before deployment, supply an attach projection, identities and operation-level rwx checks, message bounds and cancellation. Teaching callbacks are synchronous. For slow I/O consult libstyx deferred-read contracts rather than silently blocking unrelated clients.

## Acceptance

```sh
ctest --test-dir build --output-on-failure  # Run the built examples and regression checks.
```

Check normal requests, short buffers, invalid names, fragmented input, disconnect and release of held objects. Test transport coverage does not replace testing your physical driver.
