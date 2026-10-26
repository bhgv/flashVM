# FIR v1 tutorials

[Русский](README.ru.md). [Build](../README.md), [coverage](../doc/COVERAGE.md).

| Folder | Demonstrates |
| --- | --- |
| [as1](as1/README.md) | Numeric source, precedence, expected FIR |
| [as2](as2/README.md) | Parentheses and multiplication |
| [as3](as3/README.md) | Restricted numeric profile, fractional values |
| [limbo](limbo/README.md) | Int-returning module and explicit trace harness |
| [control_flow](control_flow/README.md) | if/jump and SSA dominance |
| [channels](channels/README.md) | Existing FlashVM channel extension from FIR |
| [profiles](profiles/README.md) | Source profiles, hexadecimal/fractional constants and f64.div |

Every folder has commented inputs, expected trace and bilingual explanations.
Source profiles additionally carry expected readable FIR. See each README for
commands and API parameter meanings. The suite checks source-generated and
checked-in FIR independently. `python fir/v1/examples/view_docs.py` opens this
index; the workspace viewer includes the same files.


[Building examples for the host or another OS/CPU](../../../BUILDING.md).

[structural](structural/README.md) — function/block parameters, syntax profile only.

[Limbo lexical blocks / Лексические блоки](../doc/LEXICAL_SCOPES.md).

[End-to-end Limbo→FIR→Dis and C99 API](dis/README.md).

[Typed hybrid table source](tables/main.b) · [Contract](../doc/TABLES.md) · [Русский](../doc/TABLES.ru.md). This example uses our Coco/R → FIR pipeline and a hosted C reference runner, and [native Dis table execution](../doc/TABLES_DIS.md). SWF is not supported for these tables.
