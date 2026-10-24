# Literal AVM1 storage names

FIR implementation 0.5.2 fixes accepted dotted module names. The old SSA variable spelling inserted the module text literally. A module named ide.as1 therefore emitted a name interpreted as a property path and traced undefined instead of 42.

The backend now emits a reserved __fir1$ prefix, two lower-case hexadecimal digits per original module-name byte, then the value-index suffix. This is injective for dots, underscores and case; the prefix is disjoint from legacy __fir1_ storage names. SWO1 module metadata stays unchanged, so linker duplicate-module checks retain the authored name. No imports, exports or cross-module function ABI are introduced. Long names can increase action sizes; existing checked object and signed branch limits still apply. Private storage is not a public variable API.

The regression checks seven spellings, including dot/underscore alternatives, case alternatives, legacy-prefix-like names and the longest source module name. Object inspection proves distinct storage names and retained metadata. With FIR_SWL and FIR_FLASHVM_RUNNER, it executes every module and their combined linked SWF and requires trace 42 for each.

[Backend implementation](../compiler/object.c) · [Regression](../tests/module_names.py).
