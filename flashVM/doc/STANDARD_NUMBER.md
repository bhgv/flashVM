> Historical checkpoint. Актуальные изменения v0.13: [EN](STANDARD_OBJECTS.md), [RU](STANDARD_OBJECTS.ru.md).

# Number checkpoint — v0.10

The opt-in standard library now installs native Number class/prototype metadata.
The existing global `Number(value)` conversion still returns a primitive number;
`fvm_construct(context, "Number", ...)` and AVM1 NewObject create a wrapper.
Omitted arguments produce zero. Supplied arguments use the existing ToNumber
bridge, including undefined → NaN and boxed Boolean/Number unwrapping.

The prototype wraps zero. Instances have an internal primitive numeric slot,
independent of editable ordinary properties. `valueOf` returns that slot;
`toString` formats it. Methods require a Number receiver. The class exports
MAX_VALUE, MIN_VALUE (the smallest positive double subnormal), NaN,
NEGATIVE_INFINITY and POSITIVE_INFINITY. The implementation targets the project's
IEEE-754 double model. See the [AS2 reference](https://open-flash.github.io/mirrors/as2-language-reference/Number.html).

## Formatting and limits

Omitted/undefined radix means 10. Radix is converted numerically, its fractional
part discarded, and values outside the supported 2–36 interval are rejected.
Decimal formatting uses the native string bridge. Bases 2/4/8/16/32 support
finite fractions and the full binary64 range. Other nondecimal bases support
exact integer magnitudes through 2^53−1; recurring fractions or larger values
return [FVM_UNSUPPORTED](../include/flashvm/flashvm.h#L46). Digits above 9 are lowercase; zero and nonfinite values
are handled explicitly. Bad receivers/radices
currently return C result codes; standard TypeError/RangeError objects remain
unfinished. This is not a complete version-dependent Flash formatter.

## Roots and transfer

The captured class, prototype links and internal slot are traced/preserved.
Removing the Number global does not invalidate its registered constructor after
GC. Native instanceof recognizes Number class metadata. Existing graph packets
now recognize kinds 24/25 and callback IDs 401/402; older readers reject them.
Cycles, aliases and imported prototype identity behave as for Error.

Queue transfer and file transfer between independent processes preserve the
numeric slot, valueOf and toString without installing the standard library at
the destination. Import does not register a destination global constructor;
an imported instance matches its own imported class, not another installed class.

Installation is staged with the other standard objects and commits globals only
when all stages succeed. Allocation-injection tests check that Number is absent
alongside Math/Boolean/Error after failed installation, followed by GC and retry.
See tests/standard_number.c and the extended tests/error_transfer_process.c.

Missing acceptance work includes first-class Function behavior, property
attributes, primitive receiver dispatch, complete generic ToNumber coercion,
nondecimal fractional formatting and actual Windows/macOS/Android execution.

## v0.39 update / Обновление

Power-of-two bases now support finite fractions and the full binary64 range;
other nondecimal bases retain the exact-integer limitation. See NUMBER_RADIX.md.
Основания 2/4/8/16/32 теперь поддерживают дроби и полный диапазон binary64;
для остальных недесятичных оснований сохраняется ограничение точными целыми.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_construct` | function | [include/flashvm/value.h:158](../include/flashvm/value.h#L158) |
| `FVM_UNSUPPORTED` | enum constant | [include/flashvm/flashvm.h:46](../include/flashvm/flashvm.h#L46) |
<!-- source-links:end -->
