# Blocking channel and Sys example

`main.b` comments every source line. `init()` loads the real Sys builtin, creates
an integer channel, spawns a producer and receives two blocking messages. The
expected output is `PASS Dis channels: 42`.

Build from the repository root:

```sh
bash build-workspace.sh --projects disvm --jobs 4
```

The workspace builds the bundled native Limbo bootstrap and installs
`out/disvm/console-check.dis` plus `console-check.sbl` debug information. The
bootstrap compiler is an interim build tool; the project's own Coco/R-to-FIR
Limbo compiler remains the intended compiler path.

Run `out/disvm/disvm-check --all` for the independent native interpreter/JIT
acceptance program. It does not run this `.dis` module. Running this example
requires a complete initialized Dis module host that exports actual `$Sys` and
invokes the parameterless `init()` interface. The native general-purpose host is
still being integrated; no external emu is silently required by the build.
