# A captured function on a native worker

[main.c](main.c) uses the public C89 embedding API. It creates a lexical
`answer=42`, creates AVM1 bytecode equivalent to `return answer`, snapshots it
with [`fvm_worker_run_function`](../../../include/flashvm/worker.h#L38), destroys the entire sender, then joins. Output:

```
Detached captured answer: 42
```

The eight-byte Push payload is a string type byte, six ASCII letters and a NUL.
GetVariable resolves the copied lexical environment; Return supplies a number.
The environment is pinned before later VM allocations. A successful launch owns
all copied transport data, so no sender object must survive until join.
The cleanup path joins any launched worker before destruction. The example
blocks deliberately; an IDE should poll result and continue processing input.

In a standalone FlashVM CMake build with examples enabled, build/run
`flashvm_examples_embedding_worker_function_main`. Link applications to the
`flashvm` target so configured transitive libraries and platform flags follow.
The ordinary regression is `flashvm_tests_worker_function`; it also checks four
independent mutations, cycles, native/proxy refusal and instruction limits.

This does not compile the AS `asinc` keyword or merge changes between captures.
See the complete [contract](../../../doc/WORKER_FUNCTION.md) and
[Russian contract](../../../doc/WORKER_FUNCTION.ru.md) before using large graphs,
native attachments or custom resources.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_worker_run_function` | function | [include/flashvm/worker.h:38](../../../include/flashvm/worker.h#L38) |
<!-- source-links:end -->
