# MDI/SDI membership example

[Русский](README.ru.md) · [Contract](../../../MDI-SDI.md)

The fully commented [acceptance program](../../tests/idemdi_test.b) starts one
real Limbo registry owner, registers source and graphics documents, creates a
source-only container, joins/detaches a child and dissolves the container.
It verifies stale refusal and that returned arrays cannot mutate private state.
No renderer or target is mocked; this is a registry example, not a graphical
container implementation. All documents remain on one logical IDE desktop.

In the installed editor package the same source is `examples/mdi/acceptance.b`:

```sh
./limbo-native -g -I include/limbo -o mdi-check.dis examples/mdi/acceptance.b
./disvm --stats --test-control --jit mdi-check.dis
./disvm --stats --test-control --no-jit mdi-check.dis
```

`--test-control` is explicit test-only capability. Positive native counters in
JIT mode distinguish real leaf execution from the interpreter run. Expected
output begins `PASS MDI membership`. The compiler must be the shipped NC64
compiler matching these modules; a classic four-byte compiler is incompatible.
