# C99 compiler projects

Version 0.1.1. [Русский](README.ru.md).

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
include dependency. No compiler emits bytecode/SWF yet; see each coverage table.
The separate [linker project](../linker/README.md) is a reserved project boundary.

For detached distribution, carry the common root MIT LICENSE and any generated
upstream notices. Root workspace orchestration and Git history are optional.

[AS3 core syntax subset](as3/README.md), standalone C99/Coco-R; no ABC backend.

[Source-to-SWF workflow and help](../fir/v1/doc/PIPELINE.md).
