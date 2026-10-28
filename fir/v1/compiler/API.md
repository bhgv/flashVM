# Embeddable C APIs 0.5.4

[FIR API](../include/fir.h) works without argv, stdout/stderr or subprocesses.
CLI drivers only open files, select operations and present returned diagnostics.

| API | CMake target | Operation |
| --- | --- | --- |
| `fir_source(input, output, language, module_name, error, capacity)` | `fir1_source_api` | AS1/AS2/AS3/Limbo numeric subset → FIR |
| `fir_run(task, input, output, options, error, capacity)` | `fir1_ir` | `FIR_CHECK`, `FIR_PRINT`, `FIR_OPT`, `FIR_OBJECT`, `FIR_DIS` |
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

## Direct Dis API 0.6.0

`fir_dis(output, module, error, capacity)` emits NC64 Dis for a verified
scalar i32/bool/f64/i64 CFG profile. `fir_run(FIR_DIS, ...)` uses the same implementation,
including optional optimization/snapshots. Coverage rejection preserves the
stream; NULL diagnostics are admitted; I/O failures can leave a prefix.
[Exact target and limits](../doc/DIS_BACKEND.md),
[in-process client](../examples/dis/main.c).

## CFG and decisions 0.7.0

`FIR_DIS` supports verified i32/bool blocks, comparisons and CFG edges. For N
live O_TRACE instructions, N values precede N execution flags. Count N through
the post-optimization `after` callback and publish metadata only on success.
`fir_source` admits terminal Limbo if/else; FIRDBG1 adds compare/branch events
with operator/if spans. The verifier rejects interleaved live blocks and an entry
hidden by tombstones. [Contract](../doc/DIS_BACKEND.md).

## Binary64 Dis 0.8.0

The same `fir_dis` / `fir_run(FIR_DIS, ...)` API now admits f64 constants, copy,
and add/subtract/multiply/divide. Byte output is identical to the CLI; ownership,
serialization and rejection contracts are unchanged. Numeric AS1/2/3 source
profiles emit explicit f64; dynamic coercion and strings remain rejected.
Real capture cells retain binary64 bits; executed flags remain native words.
The caller supplies type knowledge; no self-describing debug/type map is implied.

Те же C99 API принимают f64-константы, copy и четыре арифметические операции.
Типы захватов известны клиенту из FIR; флаги исполнения остаются целыми.
Правила владения, сериализации вызовов и отказа до записи сохранены.

## Homogeneous Limbo real / Однородный Limbo real, 0.9.0

`fir_source*` still selects FIR_LIMBO. Its Coco/R result-type productions choose
i32 or real; there is no extra CLI/API switch. Source metadata is emitted only
after successful staged parsing, so interface/type mismatch preserves caller
FIR and debug streams. FirSemantics appends FIR_LIMBO_REAL without renumbering
older values. The C99 regression alternates real/failure/int/AS eight times in
one process and verifies metadata, borrowed streams and direct Dis emission.

Профиль задаёт исходная грамматика, вызов остаётся FIR_LIMBO. Несовпадение типов
интерфейса/определения не публикует ни FIR, ни debug. Вызовы по-прежнему должны
сериализоваться хостом; добавление real не делает парсер реентерабельным.

[FIR 0.17: big scalar CFG](../doc/BIG_CFG.md) · [Русский](../doc/BIG_CFG.ru.md).

[FIR 0.18: exact big comparisons/source decisions](../doc/BIG_DECISIONS.md) · [Русский](../doc/BIG_DECISIONS.ru.md).

[FIR 0.19: acyclic owner CFG and fir_acyclic](../doc/TABLE_CFG.md) · [Русский](../doc/TABLE_CFG.ru.md).

[FIR 0.20: typed shifts and explicit int counts](../doc/BIG_SHIFTS.md) · [Русский](../doc/BIG_SHIFTS.ru.md).


Compiler 0.41 keeps these API signatures and serial-request rules while adding
[scoped function flow](../doc/FUNCTION_FLOW.md) to the Limbo big/table profile.
The reused C99 service also verifies recovery after a joined-path initialization error.

[Source aliases / Псевдонимы исходника](../doc/SOURCE_ALIASES.md): FIR 0.51, serialized request-local binding.
