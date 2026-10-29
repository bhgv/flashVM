# Our Limbo → FIR → Dis tutorial

[Русский](README.ru.md) · [Target/limits/API](../../doc/DIS_BACKEND.md) ·
[Commented source](main.b) · [Equivalent FIR](main.fir) · [C99 library client](main.c).

The example proves a working bounded path through our own Coco/R source
compiler, verifier and C99 Dis emitter. `main.b` starts with INT32_MAX, wraps
addition and multiplication, then subtracts one to produce **2147483647**.
Each intermediate operation wraps even though NC64 stores words in eight bytes.
`main.fir` spells out the same operations and remains usable with the SWO target.

From the repository root, after building FIR and our DisVM separately:

```sh
mkdir -p build/tmp
export TMPDIR="$PWD/build/tmp"
build/fir/fir1_source limbo --input fir/v1/examples/dis/main.b build/dis-example.fir
build/fir/fir1 dis build/dis-example.fir build/dis-example.dis
build/editor/disvm --jit --stats --inspect-words 0 1 build/dis-example.dis
build/editor/disvm --no-jit --stats --inspect-words 0 1 build/dis-example.dis
```

The `build/fir` and `build/editor` directories are explicit examples; use your
actual tool paths. `DATA 0 2147483647` is the captured first trace, not a static
success message. The compiler does not run the program to obtain that value.
No temporary Inferno compiler or assembler participates in this pipeline.

To demonstrate in-process embedding, run the built `fir1_dis_example` instead:

```sh
build/fir/fir1_dis_example fir/v1/examples/dis/main.b build/dis-example.dis
build/editor/disvm --jit --inspect-words 0 1 build/dis-example.dis
```

`main.c` calls `fir_source` and `fir_run(FIR_DIS)` directly. It owns the temporary
FIR/Dis streams, preserves existing output until compilation succeeds, checks
I/O and closes every opened stream. The exported libraries print nothing and
do not terminate the process. For an IDE, execute this blocking work on its
build worker and send copied results to the channel-owned UI.

The linear example uses i32 constants/copy/wrap arithmetic and captured
trace/return. The target also admits bool and verified CFG, as shown below. It has no imports, exported Limbo `value` ABI, strings, source
channels or Dis SBL. The source tutorial return becomes FIR `@main` trace capture;
ordinary execution without `--inspect-words` does not print it. It is not yet
possible to compile the complete graphical IDE with this subset.

Inside the installed FIR package, rebuild the C99 client using only its SDK:

```sh
cc -std=c99 -I include examples/dis/main.c lib/libfir1_ir.a lib/libfir1_source_api.a lib/libfir1_io.a -o dis-client
./dis-client examples/dis/main.b answer.dis
```

## Terminal decisions and capture flags

[decision.b](decision.b) uses our Coco/R source grammar; [decision.fir](decision.fir)
spells out its CFG. Overflow makes wrapped negative, so the outer >= arm is not
visited and the nested equality selects 42. All arms return: no SSA join or
binding mutation inside a branch is implied.

```sh
build/fir/fir1_source limbo --input fir/v1/examples/dis/decision.b build/decision.fir
build/fir/fir1 dis build/decision.fir build/decision.dis
build/editor/disvm --jit --max-instructions 1000 --inspect-words 0 6 build/decision.dis
```

Three static trace points reserve values `[0, 42, 0]` followed by executed flags
`[0, 1, 0]`. The selected value is in byte 8, and its flag is at byte 32. A real
return of zero would have flag one; a zero-filled unvisited slot has flag zero.
Slots are in physical FIR order and keep their latest value if revisited, rather
than forming a chronological log. After optimization, count trace points again:
unreachable arms and their slots can disappear.

The same source can still use `fir1 object` and SWL to produce SWF8. The source
return harness traces the selected value there; Dis captures it for inspection.
Six comparisons are supported only in terminal, explicitly braced if/else trees.
General statements inside arms, continuation after a decision, source while/for,
phi, module imports and Dis SBL remain future work.

For installed-package acceptance:

```sh
python3 tools/test_installed_workspace.py --prefix out --scratch build/package-tmp --fir-dis-only
```

This relinks the shipped C99 client from package headers/libraries, compares its
Dis bytes with CLI output, and executes both linear and conditional examples in
interpreter/JIT modes from a relocated path containing spaces.

## Binary64 AS source

[reals.as](reals.as) is a fully commented numeric AS3 example. Our Coco/R
frontend creates FIR and our C99 emitter writes Dis without an Inferno emitter.
The embedding [main.c](main.c) accepts optional third argument `as3`; omitting it
retains Limbo. Both routes use the same public API and backend.

```sh
build/fir/fir1_source as3 --input fir/v1/examples/dis/reals.as build/reals.fir --module dis_example
build/fir/fir1 dis build/reals.fir build/reals.dis
build/fir/fir1_dis_example fir/v1/examples/dis/reals.as build/embedded-reals.dis as3
build/editor/disvm --jit --inspect-reals 0 3 build/reals.dis
build/editor/disvm --no-jit --inspect-reals 0 3 build/reals.dis
build/editor/disvm --jit --inspect-words 24 3 build/reals.dis
```

```text
REAL 0 -11.5
REAL 8 3.75
REAL 16 -1.25
```

The first three eight-byte cells are values; offsets 24/32/40 are integer
executed flags, all one. The two views use separate executions of this
deterministic numeric program. Rerunning a general program with input/effects
is not an atomic combined snapshot. Types are supplied by the client, not inferred
from stored bits; inspection options are mutually exclusive. Optimization may
change trace counts, so derive offsets from the selected FIR.

For embedding, main.c stages FIR and Dis before opening the destination and
closes every stream on both paths. IDE callers must serialize parser calls on
a build worker and transfer copied results through channels. This native C
client is not claimed to run on DisVM. Installed SDK acceptance relinks main.c
using only shipped headers/archives and byte-compares its output with the CLI
after moving the whole package to a path with spaces.

## Limbo real

[real.b](real.b) is a fully commented `value(): real` profile. Coco/R checks
that interface and definition agree; all local values remain real. Use decimal
fraction/exponent literals (`2.0`, `1e2`): integer/radix/hex literals and mixed
int/real bindings are rejected rather than implicitly converted.

```sh
build/fir/fir1_source limbo --input fir/v1/examples/dis/real.b build/real.fir --module dis_example
build/fir/fir1 dis build/real.fir build/real.dis
build/fir/fir1_dis_example fir/v1/examples/dis/real.b build/embedded-real.dis
build/editor/disvm --jit --inspect-reals 0 1 build/real.dis
build/editor/disvm --no-jit --inspect-reals 0 1 build/real.dis
```

```text
REAL 0 42.5
```

FIR retains `source_language limbo` and the new `source_semantics limbo_real`.
The first line remains FIR 1.0; the new profile needs an updated reader. Existing
int programs retain `limbo_i32`. The executed flag is word offset 8; read it with
`--inspect-words 8 1`. Separate views are separate executions, not one atomic
combined snapshot.

This example covers typed/inferred real locals, division and assignment to an
outer lexical binding. Real comparisons/decisions, mixed types, casts, function
arguments and imports are not implemented. Existing Limbo int retains wrapping
arithmetic and terminal decisions. The C99 SDK client uses its original invocation
without a third argument: type selection belongs to the grammar, not the filename.

[Full-width big CFG / Полноразрядный big CFG](../../doc/BIG_CFG.md) · [FIR](big-cfg.fir).

[Big source if/else / Исходные решения big](../../doc/BIG_DECISIONS.md) · [Комментированный Limbo](big-decisions.b).

[Signed32 bit operations / Побитовые операции int](../../doc/INT_BITS.md) — FIR 0.52, SWL 0.3.12.

[int-complement.b](int-complement.b): runtime table load → explicit int narrowing → direct signed32 complement → big result 42. Every source line is commented. / Загрузка из таблицы → явное сужение в int → прямое дополнение signed32 → результат big 42. Каждая строка исходника прокомментирована.

### Table-backed complement

`int-complement.b` deliberately returns big to select the typed table profile. The table stores -43 as big; the explicit Number cast narrows the loaded value to int. The alias Number adds no runtime object. Unary complement then operates directly on signed32 bits: -43 xor -1 is 42. Only the exported result widens back to big. A table load prevents ordinary constant folding from hiding the runtime path.

Compile it with `build/fir/fir1_source limbo --input fir/v1/examples/dis/int-complement.b build/complement.fir -g build/complement.json`, then emit with `build/fir/fir1 dis build/complement.fir build/complement.dis`. Run both `build/runtime/disvm --jit --quantum 1 --stats --inspect-words 0 2 build/complement.dis` and the same command with `--no-jit`. The result slots must be 42 and 1; the latter records that the trace ran. Inspect FIR for i32.xor and no big.not. Repeat after `fir1 opt`. GC-at-each-step acceptance must finish without live table/key owners. This hosted-table example targets Dis; use int-bits.b for SWF.

[Typed predicates EN](../../doc/TYPED_PREDICATES.md) / [RU](../../doc/TYPED_PREDICATES.ru.md) describes [typed-predicates.b](typed-predicates.b) (table-backed Dis result 42) and [booleans.fir](booleans.fir) (scalar Dis/SWF bool result 1). Every nonblank source line is commented. / Каждая непустая строка прокомментирована; руководство объясняет типы, команды и проверку результата.

[Real conditions EN](../../doc/REAL_CONDITIONS.md) / [RU](../../doc/REAL_CONDITIONS.ru.md) explains [real-conditions.b](real-conditions.b) (42.5 through source → FIR → Dis/SWF) and [real-unordered.fir](real-unordered.fir) (three false IEEE predicates). Every nonblank source line is commented. / Подробное руководство объясняет типы, вычисление ветвей, NaN и команды проверки; каждая непустая строка прокомментирована.

[Lazy numeric conditions / Ленивые числовые условия — FIR 0.55](../../doc/NUMERIC_LOGIC.md) · [Русский](../../doc/NUMERIC_LOGIC.ru.md).

[Grouped numeric conditions / Группировка числовых условий — FIR 0.56](../../doc/GROUPED_CONDITIONS.md) · [Русский](../../doc/GROUPED_CONDITIONS.ru.md).

[Numeric cells and structured source flow / Числовые ячейки и структурный CFG — FIR 0.57](../../doc/NUMERIC_FLOW.md) · [Русский](../../doc/NUMERIC_FLOW.ru.md).

[Numeric counted loops / Числовой for — FIR 0.58](../../doc/NUMERIC_FOR.md) · [Русский](../../doc/NUMERIC_FOR.ru.md).

## Numeric counted loops / Числовой for

- [numeric-for-int.b](numeric-for-int.b): continue runs the staged postfix step;
  break skips it; the continuation returns 42.
- [numeric-for-real.b](numeric-for-real.b): six contributing passes return exact
  binary64 42.5, with identical raw/optimized Dis and linked SWF behavior.
- [numeric-for-nested.b](numeric-for-nested.b): independent nested steps and
  three lexical bindings named i preserve the outer 42 and sum 21 additions.

Все непустые строки этих примеров прокомментированы. Переменные заголовка
не выходят за цикл, а тело может затенить их без изменения адресата шага.
Подробные команды, ожидаемые capture-биты, ограничения backend и библиотечный
путь: [English](../../doc/NUMERIC_FOR.md) / [Русский](../../doc/NUMERIC_FOR.ru.md).

## Literal-count shifts in SWF

[int-shifts-swf.b](int-shifts-swf.b) is fully commented and returns 42 through
raw/optimized Dis and SWF. A dynamic loop input is shifted by literal two;
negative right shift preserves the sign. [Build commands and count restrictions](../../doc/SWF_INT_SHIFTS.md).

## Signed division and remainder in SWF

[int-divmod-swf.b](int-divmod-swf.b) reconstructs every integer from -7 to 7
as `(i/3)*3+i%3` and returns 42. Every line is commented. Both raw and optimized
Dis/SWF are supported. [Commands, precision proof and limits](../../doc/SWF_INT_DIVMOD.md).

## Paired integer operations

[integer-pairs.b](integer-pairs.b) restores i three ways in a dynamic loop and
returns42. Every line is commented. [Exact rules, API/CLI use and boundaries](../../doc/INTEGER_PAIRS.md).

[integer-facts.b](integer-facts.b): known-bit dynamic shift/divisor tutorial; raw/optimized Dis and SWF return42. [Guide / Руководство](../../doc/INTEGER_FACTS.md).
