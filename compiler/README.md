# C99 compiler projects

Version 0.6.0. [Русский](README.ru.md).

| Project | Current component |
| --- | --- |
| [AS1](as1/README.md) | [Syntax frontend](as1/frontend/README.md) |
| [AS2](as2/README.md) | [Syntax frontend](as2/frontend/README.md) and [display profile → FIR](as2/view/README.md) |
| [AS3](as3/README.md) | [Syntax frontend](as3/frontend/README.md) |
| [Limbo](limbo/README.md) | [Syntax frontend](limbo/frontend/README.md) |

Each language and each frontend configures independently with CMake. The optional
CMakeLists.txt here builds all four executables; the explicit external generator
options are the same as in each frontend guide. The projects use C99, not the VM's
C89/C++98 ABI/implementation rules. There is no shared C runtime or sibling-source
include dependency. These four frontends recognize syntax. The separate numeric
[fir1_source adapter](../fir/v1/doc/SOURCE_SYNTAX.md) emits FIR; fir1 compiles
it into SWO objects and [SWL](../linker/README.md) links SWF.

For detached distribution, carry the common root MIT LICENSE and any generated
upstream notices. Root workspace orchestration and Git history are optional.

[AS3 core syntax subset](as3/README.md), standalone C99/Coco-R; no ABC backend.

[Source-to-SWF workflow and help](../fir/v1/doc/PIPELINE.md).

[Syntax series, examples and API](SYNTAX.md).

[ATG layout and style checks](../tools/README.md).

C99 sources use four-space indentation, expanded blocks and visible error
handling. The component-owned [.clang-format](.clang-format) survives detached
use. Generated Coco/R code and upstream libraries are excluded.

[Complete example catalog](examples.md).

[Build for the host or another OS/CPU](../BUILDING.md).

[All five C APIs in one process](PARSER_COVERAGE.md).

[0.6.0: Unicode, regexp, CONFIG, Vector, generator boundary](LEXICAL_PROFILE.md).

[Checked int32 shifts in SWF: literal-count contract and executable example](../fir/v1/doc/SWF_INT_SHIFTS.md).

[Signed int32 SWF div/mod and proven-total arithmetic DCE](../fir/v1/doc/SWF_INT_DIVMOD.md).

[Paired integer rules / парные целочисленные правила](../fir/v1/doc/INTEGER_PAIRS.md), [RU](../fir/v1/doc/INTEGER_PAIRS.ru.md).

[Known int32 bits and dynamic checked SWF arithmetic](../fir/v1/doc/INTEGER_FACTS.md): shared C99 proofs, preserved producer errors and source example.
