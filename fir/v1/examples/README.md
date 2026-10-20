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
