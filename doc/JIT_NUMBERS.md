# Native numeric literals and unary operations — v0.351

[Русское описание](JIT_NUMBERS.ru.md)

The compiler accepts ActionPush float and double literals, plus ToInteger,
ToNumber, ToString, TypeOf, Increment, Decrement and Not. It emits ordinary calls
to shared versioned C services, retaining source checkpoints and native branches.
All architecture encoders consume the same call plan; foreign-target runtime
acceptance remains separate.

`src/avm1/number_decode.hpp` contains the arithmetic IEEE decoding previously
inline in the interpreter. Both the interpreter and compiler use these exact
formulas. Binary32 is read from one little-endian word; AVM1 binary64 stores the
high word first, with little-endian bytes inside each word. No unaligned casts,
native float representation or 64-bit integer extension is required. Signed zero,
subnormal/normal values, infinities and canonical quiet NaNs keep the prior policy;
NaN payload identity is not promised. Malformed literal widths fail compilation
without replacing the caller's code handle.

Unary operations retain the input in the native invocation's GC-rooted vector
through coercion, callbacks and string allocation. The shared service's result
replaces the top operand only after success, without an extra vector allocation.
Loaded missing operands become undefined; host-only execution remains strict.
The helper accepts semantic selectors rather than encoded Action identifiers.

`jit_numbers` checks twenty raw IEEE fixtures, including signed zero, smallest
subnormal/normal values, largest subnormal/finite values, infinities and NaNs.
Expected literal values are independent numeric fixtures. All seven unary
operations are compared for ten primitive inputs and collecting/throwing object
coercions, in host and SWF4–8 profiles. Each case runs interpreted, with an explicit
native body and with automatic JIT; native-entry counters rule out fallback.
Literal groups use separate contexts so they do not exhaust the intentional
32-entry automatic cache. Missing-operand behavior is also compared.

`jit_numbers_allocation` walks allocation failures through every new unary
operation and both numeric formats, checking native active counts, GC roots and
activation cleanup. `jit_invalid` adds truncated float/double records with stable
output-handle assertions. Nine focused minimal-profile heap/global ASan+UBSan
checks pass. Stack ASan and LeakSanitizer remain disabled for the documented
environment restrictions; see [JIT_REFERENCES.md](JIT_REFERENCES.md).

Register/constant-pool Push, function definitions/calls, object/array construction,
exception blocks and other unsupported operations still fall back to interpretation
in automatic mode. Full AVM1 coverage, full reference equivalence and real target
OS execution remain open; no roadmap stage is closed by this checkpoint.

Linux Release final full runs pass directly: AGG 706/706, Cairo 706/706 and minimal
687/687. Intermediate runs required generated-ELF permission repair and exposed
an outdated fallback fixture: execution_modes used ToNumber as an unsupported
operation. It now uses the still-unsupported ConstantPool record, while jit_numbers
explicitly verifies native ToNumber. No behavioral failure was discarded as a
permission error or hidden by retries.
