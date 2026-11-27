# fvm_worker_run_function_args

[Русский](WORKER_FUNCTION_ARGS.ru.md) · [Worker functions](WORKER_FUNCTION.md) ·
[Declaration](../include/flashvm/worker.h) · [Implementation](../src/capi/worker_function.cpp)

Include `flashvm/worker.h`. Pass the owner context, configuration and
AVM1 function required by [`fvm_worker_run_function`](../include/flashvm/worker.h#L38). Supply `count`
positional values in the borrowed `args` array, from zero to
[`FVM_WORKER_FUNCTION_INPUTS`](../include/flashvm/worker.h#L42). A zero count allows a null array. The
arguments and the closure are serialized together on the owner thread.
Repeated references to one serializable object still refer to one copy
inside the worker.

On success, `out` contains an owned worker started on a separate OS
thread. Join it before destroying the handle. `out` is cleared on error.
Invalid counts or values, unsupported native attachments and graph codec
or thread creation failures can reject launch. Execution errors and
primitive results follow the ordinary worker contract.

Pass `0, NULL` to obtain the original no-argument call behavior. Use
[Async.run](ASINC_RUN.md) for the corresponding ActionScript interface.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_WORKER_FUNCTION_INPUTS` | macro / define | [include/flashvm/worker.h:42](../include/flashvm/worker.h#L42) |
| `fvm_worker_run_function` | function | [include/flashvm/worker.h:38](../include/flashvm/worker.h#L38) |
| `fvm_worker_run_function_args` | function | [include/flashvm/worker.h:53](../include/flashvm/worker.h#L53) |
<!-- source-links:end -->
