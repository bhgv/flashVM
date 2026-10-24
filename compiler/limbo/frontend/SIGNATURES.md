# Function signatures and the first binding check

[Русский](SIGNATURES.ru.md) · [Catalog](INTERFACES.md) ·
[Shared Dis/AS module examples](../examples/external-modules/README.md)

Version 0.8.0 adds [signature.h](signature.h) and [signature.c](signature.c).
These consume the existing Coco/R production tree. There is no second parser,
regex grammar, native Limbo invocation or implicit filesystem lookup.

`limbo_interface_signature(owner, declaration, &view)` reports the exact function
Type node, direct Parameters/result nodes, expanded fixed argument count,
variadic star and syntactic polymorphic parameter presence. An omitted result is
`SIZE_MAX` (void). Grouped exports share their authored Type node. Nested callback
parameters/results never become parameters/results of the enclosing function.

`limbo_interface_parameter` indexes fixed arguments, expanding `a,b:int` to two.
`limbo_interface_parameter_next` traverses them without repeatedly scanning earlier
formals: start with `SIZE_MAX`, then pass the previous returned `name_node`.
The returned name byte span, Type/NamedType node and self flag borrow the retained
catalog's tree. Repeated `nil` placeholders remain separate positions. A variadic
star is not a fixed argument. Numeric indices always belong to this exact owner;
they are not cross-module identities or native frame offsets.

`limbo_interface_type_kind` classifies a Type's grammar-selected outer shape:
byte/int/big/real/string, array/list/chan/ref/fn, tuple/fixed/named. It deliberately
leaves aliases named and does not recursively resolve nested types. A self formal
exposes a NamedType rather than Type and is reported separately. Every query is
allocation-free, uses no parser globals and preserves caller output on failure.
Owners must remain alive and destruction must be synchronized with readers.
Positional lookup scans earlier formals; use the cursor for sequential generation.

`limbo_interface_validate_functions` is a separate, executable semantic step:
fixed parameter bindings must be unique within each function. Repeated `nil`
placeholders are permitted. Per-function sorted UTF-8 keys give O(P log P)
comparison work and O(P) temporary storage. Failure -9 identifies the earliest
repeated parameter spelling in the first affected source signature. -2 reports
allocation failure, -1 a null owner. Diagnostics count original UTF-8 bytes,
with one-based LF-delimited lines/columns. The owner is unchanged on any result.
This is not full type checking: receiver eligibility, polymorphic constraints,
aliases/imports/includes, ADT layouts and native structural signatures remain open.

## Generator integration

`limbo_bindings` runs this binding check before opening output, for every target.
A duplicate parameter therefore preserves an existing output file, just like a
syntax error. JSON declarations now contain an additive `signature` field (null
for nonfunctions). Functions include `fixed_count`, `variadic`, `polymorphic`,
ordered parameter names/outer kinds/self flags/tree nodes/source byte offsets,
and a result kind or `void`. Existing schema/format and source snapshot are kept.
AS1/2/3 provider/proxy descriptors contain the same structure.

Generated AS providers reject argument count/scalar mismatches before calling a
handler and validate its primitive result before success. Proxies reject invalid
arguments before sending or consuming an ID; an invalid primitive successful reply
finishes that call once with an error. A duplicate later reply stays ignored.

| Kind | Common AS guard |
| --- | --- |
| int | Number, integral, signed 32-bit range; NaN/infinity rejected |
| byte | Number, integral, 0..255 |
| real | Number; IEEE NaN/infinity allowed |
| string | String or null/undefined nil |
| void result | null/undefined |
| big, named, composite, callback | Host must resolve/check/marshal; no invented AS representation |

Variadic functions check their fixed prefix and minimum count. Named polymorphic
arguments remain unresolved. These guards neither authenticate an endpoint nor
replace a real native Dis/SWF marshaler, heap ownership, GC roots or official
signature checking. `big` must never silently become a lossy AS Number. C callbacks
remain opaque and explicitly unimplemented; Python remains optional host tooling.

## Validation

The standalone C99 tests cover grouped exports/formals, nested callbacks, tuples,
variadics, polymorphism, self, comment-separated names, null/invalid queries,
retained owners across parsing, Unicode duplicate diagnostics, repeated nil and
allocation failure followed by reuse of the same owner. Generator checks cover
metadata and output preservation. All three Coco/R AS parsers accept the generated
adapters. Optional Node executes their actual common-AS source with a Dis transport
double; it does not execute SWF or prove a native module bridge.
