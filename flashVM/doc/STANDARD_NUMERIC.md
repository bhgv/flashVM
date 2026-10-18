# Initial numeric standard library

Include `flashvm/standard.h` and call `fvm_standard_install(context)` before
executing the movie. Installation is opt-in: old embedding code retains its
previous global namespace and GC object counts. Existing globals, native/stack
registrations or named script functions under reserved names reject the first
installation. Globals and registrations are published together after preparation.
Repeated installation validates the Math method registrations and numeric
function registrations; it does not restore user-mutated constants.

Installed: Math, NaN, Infinity; callable-by-name Number, Boolean, isNaN, isFinite,
and a boxed Boolean constructor/prototype. Number remains a primitive conversion
function without a boxed constructor. See [Boolean](STANDARD_BOOLEAN.md).
Global functions and Math methods are not yet exposed as first-class
function-valued properties. Constants are currently mutable ordinary properties;
read-only/non-enumerable property attributes remain outstanding.

Math methods: abs, acos, asin, atan, atan2, ceil, cos, exp, floor, log,
max, min, pow, random, round, sin, sqrt, tan.
Math constants: E, LN10, LN2, LOG10E, LOG2E, PI, SQRT1_2, SQRT2.
Methods work through [fvm_object_call](../include/flashvm/value.h#L126) and AVM1 ActionCallMethod. Missing arguments
convert to NaN; extra arguments are ignored by fixed-arity methods. min/max
accept any count, return +/-Infinity for no arguments, propagate NaN, and select
the appropriate signed zero. round breaks ties toward +Infinity and preserves
-0 for values between -0.5 and 0. pow(NaN,0) is 1; pow(+/-1,+/-Infinity) is NaN.
Other elementary operations delegate to the platform C math library; cross-target
last-bit agreement has not been established.

Math.random uses a context-local unsigned 32-bit LCG and returns [0,1).
Each new context starts with seed 1, so sequences are intentionally reproducible.
Workers have independent state; no global libc rand state is shared. This is not
cryptographic randomness or a replica of Flash Player's PRNG.

[`fvm_value_to_number`](../include/flashvm/standard.h#L23) converts undefined to NaN, null to +0, boolean to 0/1,
and preserves numbers. String conversion is independent of locale: ASCII
whitespace, signed decimal/fraction/exponent, unsigned 0x hexadecimal, Infinity
and empty-string zero. Invalid syntax yields NaN. Negative zero is preserved.
Boxed Boolean converts its stored primitive to 0/1; custom overrides of valueOf
are not consulted by this interim unboxing rule. Generic object
ToPrimitive/valueOf/toString returns UNSUPPORTED. Unicode whitespace,
all SWF-version coercion differences, and correctly rounded arbitrary-length
numeric string conversion remain unfinished; the decimal parser retains up to
19 significant digits and bounded exponent counters. This is an explicit
supported subset, not full ECMAScript string conversion.

Script workers created by the thread extension snapshot the parent's opt-in
state and install their own numeric namespace. No parent Math object or PRNG
state is shared. User-defined function snapshots remain local to the worker.

Numerical edge-case reference: ECMA-262 section 15.8:
https://262.ecma-international.org/5.1/#sec-15.8 . This reference informs tests;
it does not establish full Flash<=8 compatibility.

Validation: tests/standard_numeric.c covers every Math method, constants,
NaN/infinity/signed zero, round ties, subnormal parsing, type conversion,
conflict preservation, GC and real AVM1 CallMethod. The ActionScript extension
integration test calls Math.pow inside an isolated script worker. Full standard
objects, further boxed classes and other system APIs remain in SWF8_ROADMAP.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_object_call` | function | [include/flashvm/value.h:126](../include/flashvm/value.h#L126) |
| `fvm_standard_install` | function | [include/flashvm/standard.h:18](../include/flashvm/standard.h#L18) |
| `fvm_value_to_number` | function | [include/flashvm/standard.h:23](../include/flashvm/standard.h#L23) |
<!-- source-links:end -->
