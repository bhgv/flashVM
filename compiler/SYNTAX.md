# Current parser syntax 0.5.0

[Coverage, checks and remaining work](PARSER_COVERAGE.md) describe the current
series. ASI, AS3 regexp/metadata/Vector/E4X and newer Limbo constructs are added;
extended FIR structure is separate from executable-profile verification. The
0.3.1 checkpoint below is historical; its former limits are not the current list.

# Syntax series 0.3.1

[Русский](SYNTAX.ru.md). All parsers use readable Coco/R grammars generated
by the external [bhgv/coco-r-C](https://github.com/bhgv/coco-r-C). Actions,
lowering and library wrappers are C99; generated runtime sources stay in builds.

| Component | Added syntax | Example / authoritative grammar |
| --- | --- | --- |
| AS1, AS2, AS3 | Membership `in`; scoped NoIn in for initializers | [AS1](as1/examples/in.as), [AS2](as2/examples/in.as), [AS3](as3/examples/in.as); each frontend .atg |
| AS3 | Default/final rest parameters, return void, contextual get/set and each, for-each, namespace declarations/use/access, multiple catches | [Functions.as](as3/examples/Functions.as), [as3.atg](as3/frontend/as3.atg) |
| Limbo | Labels on six control forms, nonempty list literals, dereference, array initializers with optional size and trailing comma, checked slices | [Syntax.b](limbo/examples/Syntax.b), [limbo.atg](limbo/frontend/limbo.atg) |
| Numeric source → FIR | Multiple AS bindings, AS3 const, compound arithmetic, inferred Limbo bindings, unary operators, hex/radix/fractional literals | [Coverage and examples](../fir/v1/doc/SOURCE_SYNTAX.md), [src1.atg](../fir/v1/compiler/src1.atg) |
| FIR 0.4.1 | Paired source profile fields, signed/hex numbers, `f64.div` | [Format](../fir/v1/doc/FORMAT.md), [profile example](../fir/v1/examples/profiles/main.fir) |

The four broad frontends recognize syntax. Their new constructs do not all lower
to FIR: only the separately documented numeric source profile does. FIR compiles
to SWO; SWL links objects/libraries/resources into SWF. AS3 numeric compatibility
still targets AVM1, not ABC. There is no AS4 frontend or defined AS4 dialect.

AS3 diagnostics 1201/1203 reject required parameters after optional parameters
and parameters after rest; 1202 rejects a three-clause for-each header. Limbo
1301 rejects `array[] of Type` without a size or initializer. Empty lists use
`nil`; `list of {}` is rejected. Write wildcard arrows as `* =>`: `*=>` starts
with the existing `*=` token under longest-match scanning.

Each language exposes a namespaced `*_check` / `*_read` C API; CLI drivers call
that API. Parser globals require external serialization per language. Sequential
calls, including valid input after errors, reset state. UTF-8 input is bounded
to 2 MiB. Recursive grammar boundaries have a 512-active-call limit (-3) and
4000000-visit budget (-4). Resource diagnostics may have zero coordinates.
Grammar actions wrap whole alternative groups, so every successful entry leaves
once. AS3 parameter lists are iterative rather than recursively proportional to
the parameter count.

Tests cover acceptance/rejection, comments and NoIn restoration, parameter and
aggregate diagnostics, truncated inputs with process timeouts, deep inputs and
work exhaustion followed by valid input in the same host. Source/FIR tests
separately verify profiles, immutable bindings, range checks, optimizer round
trips and actual SWO/SWL/VM traces including signed zero, infinity and NaN.

Remaining boundaries: explicit AS semicolons, no AS regexp/E4X/Vector/metadata
support in this series, no full type/symbol/context checking, no general Limbo
module/channel lowering. See each language README for its full coverage table.
