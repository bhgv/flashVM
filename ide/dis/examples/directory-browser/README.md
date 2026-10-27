# Directory owner and IDE browser tutorial

[Русский](README.ru.md) · [Protocol](../../DIRECTORIES.md) · [Source](main.b) ·
[Driver](../../../../disVM/runtime/test_directory_browser.py)

This executable example uses the same `Idefiles.startdirs` implementation as the
console IDE. It sends typed requests to a real Dis process and checks actual
Sys stat/fstat/dirread through OS ports. No emulated file service is substituted.

Requirements: matched DisVM/`limbo-native`, `sys.m`, `idefiles.m`, and built IDE
modules including idefiles.dis/editor-console.dis. The native acceptance driver
currently targets Linux; it creates an intentionally invalid-byte filename as an
independent failure oracle. Source `FIXTURE_ROOT` is a deliberate placeholder:
the driver creates private files, substitutes a quoted path, compiles, executes
and removes its own fixture. Do not compile the template without substitution.

From the repository root after the runtime build:

```sh
ctest --test-dir build/runtime -R directory_browser --output-on-failure
```

From an installed runtime SDK (the complete SDK may be relocated):

```sh
python3 examples/directory-browser/run.py ./disvm ./limbo-native \
  ./include/limbo ./include/limbo ./examples/directory-browser/main.b . jit
```

Repeat with `interpreter`. For the editor SDK use `./disvm-editor` as the first
argument. Expected final driver output: `PASS directory service and IDE browser
jit`. Each mode executes the Limbo client at quantum 1 and 2048, verifies actual
native dispatch counters, and then replays terminal IDE commands.

The fixture contains 74 entries, including Unicode names, a four-byte binary file,
an empty directory, a sparse 4294967303-byte file and a control-character name.
Sparse-file support and native metadata capacity are required. A separate invalid
UTF-8 directory checks error retirement without partial publication.

| Demonstration | Expected result |
| --- | --- |
| Metadata and large length | Exact copied Sys.Dir values, no file cursor exported. |
| Two interleaved cursors | Independent positions; changing one returned array cannot affect another. |
| Complete stream | Three batches, at most 32 entries, then stable End. |
| Close and reused slot | Old ID returns Stale; replacement uses a newer big ID. |
| Quota | Eight live identities; ninth OpenDir returns Full; StatPath still succeeds. |
| Bad requests | Unknown operation, negative correlation, bad cursor/path and URI fail explicitly. |
| Owner lifecycle | 80 explicit open/close cycles and 32 terminal cycles with eight still-open cursors. |
| Frontend replay | Failed replacement preserves old cursor; text/history stay intact; controls are escaped. |

The client releases or explicitly shuts down every owner. Records and arrays are
independent VM values; callers may mutate results and retain metadata after close.
IDs are endpoint-local, so always keep `(endpoint, id)` together. A dropped reply
is not cancellation; the sender must drain it before ending the owner.

See the [protocol](../../DIRECTORIES.md#protocol-and-ownership) for every operation,
return status, ownership rule and scheduling limitation. Native I/O still blocks
one VM owner; enumeration is not atomic or sorted. Graphical file-tree widgets,
`/prog`, network leases and asynchronous cancellation remain separate work.
