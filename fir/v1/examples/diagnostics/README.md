# C99 source diagnostic client

[Русский](README.ru.md) · [Contract](../../doc/SOURCE_DIAGNOSTICS.md)

[main.c](main.c) is a fully commented native host program linked only to the
independent FIR source API. It constructs the invalid source `/*中*/\ttrace(missing);`
with explicit UTF-8 bytes and calls `fir_source_ex`. The expected rejection is
the example's successful outcome: it checks kind, span and zero output bytes.
No invalid SWF is produced or executed.

From the repository root, after preparing the external Coco/R SDK:

```sh
./build-workspace.sh --projects fir --tests
TMPDIR="$PWD/build/workspace/tmp" build/workspace/fir/fir1_diagnostic_example
```

Expected output:

```text
source line 1: read before initialized declaration
byte span=[14,21), line=1, byte column=15
PASS source diagnostic example
```

Input/output streams belong to main and are closed on every path. The diagnostic
is copied scalar data; the compiler retains no source pointer after returning.
The seven-byte identifier starts after a three-byte character and a tab, so an
editor must convert byte offset 14 into character offset 12 before navigating.
The original input's digest and a captured live revision are needed when handling
an asynchronous result; a valid position alone does not make a late result current.
See [IDE host admission](../../../../ide/README.md) for opt-in structured results.
The example is a native build aid, not the deployed Limbo runtime.
