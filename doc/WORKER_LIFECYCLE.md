# Worker lifecycle (C89 API)

A worker owns an independent FlashVM runtime and context. The public API in
`include/flashvm/worker.h` supports immediate and suspended execution.

* `fvm_worker_run(config, fn, input, userdata, &worker)` starts immediately.
* `fvm_worker_run_suspended(...)` creates the native OS thread but blocks it
  before invoking the callback until `fvm_worker_start(worker)`.
* `fvm_worker_start(worker)` is idempotent.
* `fvm_worker_state(worker, &state)` returns a synchronized snapshot:
  `SUSPENDED`, `RUNNING`, `FINISHED`, or `JOINED`. A running worker may
  complete immediately after the snapshot.
* `fvm_worker_join(worker, &output)` starts a suspended worker if necessary,
  waits for termination and returns the callback result. Output strings are
  owned by the worker and are valid until destruction.
* `fvm_worker_destroy(worker)` must be called after join; it does nothing
  before join. Do not access a handle after destruction.

Only primitive values can be copied across contexts. Input strings are copied;
ordinary Flash objects and functions cannot be passed. `userdata` is borrowed
and must remain valid until join. Do not invoke `join` concurrently from
multiple host threads on the same handle; handle destruction is also not
thread-safe. Use channels for inter-worker communication.

This stage does **not** implement cooperative cancellation, thread priorities,
ActionScript `thread.run`, or suspension of a thread that is already running.
It does not forcibly terminate threads, because doing so can corrupt VM state.

Example: `examples/embedding/worker_lifecycle/main.c`.
