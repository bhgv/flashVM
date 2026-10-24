# Embeddable C APIs 0.5.4

[FIR API](../include/fir.h) works without argv, stdout/stderr or subprocesses.
CLI drivers only open files, select operations and present returned diagnostics.

| API | CMake target | Operation |
| --- | --- | --- |
| `fir_source(input, output, language, module_name, error, capacity)` | `fir1_source_api` | AS1/AS2/AS3/Limbo numeric subset → FIR |
| `fir_run(task, input, output, options, error, capacity)` | `fir1_ir` | `FIR_CHECK`, `FIR_PRINT`, `FIR_OPT`, `FIR_OBJECT` |
| `fir_optimise(module, error, capacity)` | `fir1_ir` | In-place verified optimization |
| `fir_object(output, module, error, capacity)` | `fir1_ir` | FIR → SWO1 |

Streams remain caller-owned and are never closed by the library. Reading and
writing start at their current positions. Return: 1 success, 0 failure. The
caller owns the diagnostic buffer (NULL/0 is allowed). `module_name` excludes @,
NULL defaults to tutorial; maximum 254 ASCII bytes. Use unique module names when
linking sources. CLI: `fir1_source as2 --input a.as a.fir --module a`.
`FirRunOptions.optimise` requests optimization before object compilation;
otherwise it is explicit. `snapshot(user, phase, module)` receives before/after
and lets an IDE save FIR using `fir_print`. The borrowed module is valid only
during the callback; return 0 to cancel. Do not recursively invoke a parser
from the callback. `fir1_opt INPUT OUTPUT` uses the same API.

FIR and source parsers have distinct symbol namespaces and link into one program.
Each call resets Coco/R errors, including calls after invalid input. Legacy
scanner fatal paths are intercepted as returned failures. Parsers are not yet
reentrant: serialize calls to each parser with an external mutex. Optimizing
distinct caller-owned modules and independent SWL contexts does not use parser
state. Do not mutate one module concurrently.

[C test](../tests/embed.c) links both compilers, repeats failures/successful
compilations of four profiles, invokes snapshots and checks SWO1 without child
processes. Generated code stays in build; reset/guard wrappers are C99 and
`.atg` remains authoritative. Broader syntax frontends are separate APIs/projects.

## Source diagnostics 0.5.4

`fir_source_ex` adds an optional caller-owned FirSourceDiagnostic result after
the existing arguments. `fir_source` remains a wrapper. First error wins,
success clears the result, positions use original UTF-8 bytes.
[English contract](../doc/SOURCE_DIAGNOSTICS.md) ·
[Полный контракт на русском](../doc/SOURCE_DIAGNOSTICS.ru.md).
