# Loaded Number nondecimal values — v0.384

Loaded SWF4–8 Number.toString with a selected nondecimal base now uses the
pinned AVM1 Int32 conversion policy. Finite values inside the signed 32-bit
range truncate toward zero. Fractions therefore have no fractional digits:
42.9 becomes 1120 in base 3, and -0.25 becomes 0. The existing radix argument
coercion/fallback rules remain. Decimal formatting and unloaded host exact
power-of-two fractions/large integer behavior stay unchanged.

The range test precedes truncation. Out-of-range values, NaN and either infinity
select INT32_MIN rather than wrapping or saturating to a positive maximum.
The pinned implementation's wrapped negation leaves INT32_MIN negative; signed
remainders then map below ASCII zero. This intentionally produces punctuation
or control bytes, such as -(0000000 in base 16, instead of a normal magnitude.
The implementation reproduces those bytes using bounded exact double/integer
operations without signed negation overflow or implementation-defined negative
division. Counted text preserves control bytes through adoption and GC.

This follows the pinned current-player reference policy. Historical Flash
Player NaN/Infinity behavior, including the documented older zero/different
garbage behavior, is not selected by a SWF version or newly certified here.
A SWF version is not a Flash Player version. No independent original-player
run was added; the fixtures encode the source-comparison policy.

[GC:] Stored numeric data is captured before radix coercion. A callback may
mutate the box, but the current result uses the captured value. Existing native
collection barriers, exception propagation and output ownership remain. Failure
and retry restore root/conversion/native state. Formatting does not change stored
data. Existing callback IDs, public ABI and graph formats are unchanged; restored
native methods use the receiver context's loaded/host policy.

Tests: number_loaded_radix checks all 34 nondecimal bases in SWF4–8 against
independent integer fixtures, signed bounds, fractions, sentinel inputs, NaN/
infinities, unchanged decimal values/stored data, actual rooted GC and boxed
transfer to loaded/host receivers without standard installation. Allocation
sweeps cover normal/sentinel/NaN conversion and GC/retry. Callback cases cover
captured values, mutation, native GC blocking, exceptions and errors.
jit_number_radix compares all bases for normal/sentinel inputs in interpreter,
explicit and automatic native execution. Two 17-body groups keep the automatic
checks below the existing 32-entry cache fallback bound.

Primary sources at pinned Ruffle 2372a4e148a67d68efd796f4477b3cddd4f880cd:
[Number to_string](https://github.com/ruffle-rs/ruffle/blob/2372a4e148a67d68efd796f4477b3cddd4f880cd/core/src/avm1/globals/number.rs),
[Int32 clamp](https://github.com/ruffle-rs/ruffle/blob/2372a4e148a67d68efd796f4477b3cddd4f880cd/core/src/avm1/clamp.rs).
All nine roadmap stages remain open.

Validation v0.384: all 803 AGG / 803 Cairo / 784 minimal Linux Release checks
have passing evidence after nine ELF execution-permission repairs and targeted
reruns (four AGG, four Cairo, one minimal). Eight restricted heap/global
ASan+UBSan checks pass: four new radix/callback/allocation/native cases plus
legacy radix/conversion, C89 standard Number and native numeric regressions.
Stack ASan and LeakSanitizer remain disabled. The new scoped radix constants
header passes a strict C89 syntax probe. No original external SWF oracle or
foreign-OS acceptance was added. All nine roadmap stages remain open.
