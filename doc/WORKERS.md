> Current status: [ActionScript API](ACTIONSCRIPT_EXTENSIONS.md) and [ownership](CONCURRENCY_SAFETY.md). The text below is an earlier-stage design document; its limitations do not describe the current API.

# Native worker API (stage 28)

`flashvm/worker.h` exposes a C89-compatible interface. C++98 callers can use the same ABI.

- `fvm_worker_run(config, callback, input, userdata, &worker)` creates an OS thread using `ports/<os>/thread.c`. The callback receives a **new** interpreter context as its first argument. Input is copied before thread creation.
- `fvm_worker_join(worker, &result)` waits for completion, returns the callback's status and optionally a borrowed result value. Repeated joins are allowed.
- `fvm_worker_destroy(worker)` releases the handle **after join**. Calling it before join has no effect, to avoid freeing memory still used by the thread.

Only undefined, null, boolean, number, and NUL-terminated UTF-8 strings are supported for inputs/outputs. Object values are rejected, including object results. Output strings are copied before the worker context is destroyed and remain valid until worker destruction. `userdata` is borrowed, not copied; its lifetime is the caller's responsibility. Each worker has a private runtime and context, so object pointers must not cross thread boundaries. A channel can be supplied through `userdata` only when its lifetime is externally synchronized. Do not destroy channels while worker threads are using them.

**Limitations:** no cancellation or suspended start, no ActionScript thread object, no function serialization, no automatic channel cloning, and no multi-worker `join` selector yet. On the tested host, the POSIX port uses pthreads. Windows/macOS/Android ports exist but are not verified here. The current API does not provide timeouts; a blocked callback can make `join` wait indefinitely.

Example: `examples/embedding/workers/main.c`.
