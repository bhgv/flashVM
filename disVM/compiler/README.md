# VM-independent compiler service library

[Contract EN](../../ide/COMPILER-SERVICE.md) / [RU](../../ide/COMPILER-SERVICE.ru.md) · [Public C99 API](compiler_engine.h) · [Actual API fixture](test_engine.c)

This library composes our existing source/FIR and SWL libraries. It owns staging
and returns complete copied artifacts/debug bytes. It includes no VM headers,
spawns no CLI process and grants no UI or document authority. The corresponding
optional native module is Fircompiler; Limbo idecompiler owns workspace jobs.

Build independently with the four explicit matching FIR/SWL include/library
paths documented in the contract. Enable DVM_COMPILER_TESTS for real compilation,
source/backend/options/budget failures, recovery, double disposal and unavailable
staging. The service is serialized and blocking; source parsers are Coco/R.
Artifact/debug budgets are independently 2 MiB and source is at most 64 KiB.
The supported dependency layout currently uses POSIX static .a archives.

Do not save, execute or relabel an output unless stage is DVM_COMPILE_OK. Dispose
any previous result before reusing its storage; input stays caller-owned.
Both returned buffers are native-owned copies and must not be installed as a
shared Dis/FlashVM heap object. The native binding copies them into mapped roots.

## Canonical binary layout (0.1.1)

Binary backends now consume reparsed canonical FIR, matching separate CLI stages.
Constant folding can remove temporary instructions while leaving original
in-memory value slots allocated; reparse gives the same slot layout to library
and CLI consumers. Original FIRDBG1 stays separate. The standalone canonical
regression compares exact Dis bytes for four languages and both options, and
export acceptance compares all three formats with independently run tools.
