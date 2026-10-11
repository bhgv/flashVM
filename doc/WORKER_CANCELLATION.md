# Cooperative worker cancellation and multi-join

The portable C89 worker API now exposes `fvm_worker_stop`, `fvm_worker_stop_requested`, and `fvm_worker_join_all`.

`stop` is a **request**, not forced termination. It marks the worker's synchronized cancellation flag and wakes a suspended worker. The worker callback must poll `stop_requested` and return cooperatively. It does not interrupt blocking I/O or a blocked channel receive. A caller must still `join` before destroying the worker. `join_all` joins every worker in its array, even if one callback fails; it returns the first error and rejects duplicate/null handles. Output strings belong to worker handles until destruction.

The worker callback receives an independent interpreter context. To poll its handle safely, create the worker suspended, publish its handle in callback userdata, then start or request stop. Never access the handle after destroy. Callback userdata is not copied. No VM objects are shared across worker contexts.

See `examples/embedding/worker_stop/main.c` for a runnable C89 example.

**Limitations:** This is not an ActionScript `thread` object. No automatic VM interruption, channel cancellation, or multi-worker completion-order iterator is implemented yet. Native worker backends are built for the selected platform; only Linux has been tested in this development environment.
