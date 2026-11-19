# Small generated-scanner example

[Русский](README.ru.md) · [Source](main.b) · [Service](../../bridge/idesyntax.m) · [Full guide](../../../SYNTAX-LAZY.md)

Menu **16** runs this console Dis module. It loads `Idesyntax`, calls
[`start`](../../bridge/idesyntax.b), submits one Limbo and one ActionScript Unicode
snapshot through the returned job channel, receives their complete catalogs and
checks `project` rejects a stale revision. The generated Coco/R scanners perform
recognition; no handwritten lexer or external compiler process substitutes for them.

```sh
sh out/disvm-editor/run-syntax-check.sh --jit
sh out/disvm-editor/run-syntax-check.sh --no-jit
```

The scripts also exist in `disvm-runtime`. The flags select actual JIT or
interpretation. No input file, GUI context or additional setup is required.

Expected output:

```text
PASS Coco/R syntax example: profiles=2 Unicode=ok stale=reject shutdown=ok
```

Read [`init`](main.b) in order: load the service, keep both channels rooted, send
one request and consume one answer at a time, validate both palettes, then send
`stop=1` and consume the matching acknowledgement. Errors raise after shutdown.
Normal completion lets the native runtime join the worker.

The separate [stress fixture](../../tests/idesyntax_test.b) tests maximum-size
inputs/token quotas and cancellation bursts. It can be substantially slower and
is not the program launched by menu 16. Running this small example does not claim
completion of that full stress matrix or a physical typing-latency benchmark.
