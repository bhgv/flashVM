# Cooperative worker cancellation and multi-join

The portable C89 worker API now exposes [`fvm_worker_stop`](../include/flashvm/worker.h#L52), [`fvm_worker_stop_requested`](../include/flashvm/worker.h#L54), and [`fvm_worker_join_all`](../include/flashvm/worker.h#L60).

`stop` is a **request**, not forced termination. It marks the worker's synchronized cancellation flag and wakes a suspended worker. The worker callback must poll `stop_requested` and return cooperatively. It does not interrupt blocking I/O or a blocked channel receive. A caller must still `join` before destroying the worker. `join_all` joins every worker in its array, even if one callback fails; it returns the first error and rejects duplicate/null handles. Output strings belong to worker handles until destruction.

The worker callback receives an independent interpreter context. To poll its handle safely, create the worker suspended, publish its handle in callback userdata, then start or request stop. Never access the handle after destroy. Callback userdata is not copied. No VM objects are shared across worker contexts.

See `examples/embedding/worker_stop/main.c` for a runnable C89 example.

**Limitations:** This is not an ActionScript `thread` object. No automatic VM interruption, channel cancellation, or multi-worker completion-order iterator is implemented yet. Native worker backends are built for the selected platform; only Linux has been tested in this development environment.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_worker_join_all` | function | [include/flashvm/worker.h:60](../include/flashvm/worker.h#L60) |
| `fvm_worker_stop` | function | [include/flashvm/worker.h:52](../include/flashvm/worker.h#L52) |
| `fvm_worker_stop_requested` | function | [include/flashvm/worker.h:54](../include/flashvm/worker.h#L54) |
<!-- source-links:end -->
