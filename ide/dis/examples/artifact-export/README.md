# Export a compiled document to new files

`main.b` explains every step inline: open an actual editor document, compile its
revision through typed channels, fetch metadata, start the exporter, explicitly
create `example.dis` and `example.firdbg`, and acknowledge exporter/panel/document
shutdown in dependency order. No subprocess or automatic execution is involved.

Run `disvm --jit artifact-export.dis` or `disvm --no-jit artifact-export.dis` from
a private writable directory with matching shipped modules and Fircompiler=ON.
The example creates those two **new** files in the working directory. Existing
names fail without overwrite; use a fresh directory for another run. Failure of
one output does not roll back the other. A failed native write may leave a partial
new file. Source/history/saved state remain untouched.

Success prints `PASS artifact tutorial: binary/debug=new revision=ok owners=closed`.
`example.dis` is a complete native-cell Dis module returning 42 when separately
executed; `.firdbg` is original FIRDBG1, not Inferno `.sbl`. A separate application
may explicitly request execution. Change target and binary suffix together for
FIR/SWF, and always validate returned target/revision before using an output.

[Export contract](../../../ARTIFACT-EXPORT.md) · [Build tutorial](../build-panel/README.md).
