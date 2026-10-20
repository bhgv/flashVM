# C99 compiler projects

Version 0.4.0. [Русский](README.ru.md).

| Project | Current component |
| --- | --- |
| [AS1](as1/README.md) | [Syntax frontend](as1/frontend/README.md) |
| [AS2](as2/README.md) | [Syntax frontend](as2/frontend/README.md) |
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
