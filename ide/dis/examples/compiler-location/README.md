# From real compiler bytes to editor character selection

[Русский](README.ru.md) · [Utf8Span contract](../../UTF8_SPANS.md) · [Client](main.b)

The complete AS input is [source.as](source.as): `/*中😀*/ trace(missing);`.
Our numeric `fir1_source` rejects the uninitialized identifier with byte offset 18,
length 7 and byte column 19. The corresponding Limbo selection is characters
[13,20), column 13, spelling exactly `missing`. Passing byte offset 18 directly to
Locate would select the wrong position. The fully commented client sends Utf8Span
through the typed document channel and rejects the same diagnostic after an edit.

[diagnostic-values.m](diagnostic-values.m) is a checked-in developer fixture.
The optional integration [source_navigation.py](../../tests/source_navigation.py)
invokes the **real FIR source compiler**, retains its exact JSON/source, checks
rejected compilation preserves an earlier artifact, regenerates the constants,
compiles the same client and executes it in the explicitly selected reference VM.
The source is not reparsed by Python; transport copies the compiler's byte span.
This host test does not put native compilers or Python into the deployed IDE.

From the root, with a matching native Limbo/reference emulator pair:

```sh
LIMBO_EXECUTABLE=/matching/bin/limbo \
LIMBO_MODULE_DIR=/matching/module \
EMU_EXECUTABLE=/matching/bin/emu \
./build-workspace.sh --projects fir,ide --tests
```

Selecting both FIR and IDE enables these integration tests automatically; selecting
IDE alone adds no hidden FIR dependency. For independent ide/dis CMake builds, add
`-DFIR_SOURCE_EXECUTABLE=/explicit/build/fir1_source` with the three usual reference
SDK variables, build and run:

```sh
ctest --test-dir BUILD -R diagnostic_navigation -V
```

Both modes must print `PASS idecore compilerlocation ... checks`. Artifacts remain
under BUILD/source-navigation-0 and -1, including the actual diagnostic JSON.
Test temporary streams use a build-owned tmp directory. Compiler/headers/emulator
must share an ABI; root disVM is currently a core archive, not a compatible runner.

For only the checked-in fixture, manually compile main.b with includes for this
folder, ide/dis and a matching sys.m, place that module and idecore.dis in an
isolated emulator namespace and run compilerlocation.dis as its init module.
The bootstrap deliberately halts that private emulator after its report; it must
not be used as init in an interactive user session.

The source revision and service identity are mandatory association metadata.
A real UI must retain them when submitting compilation and use the reply only for
that source. This example does not implement a compiler process manager, debug-PC
relocation, native Dis/SWF binding or rendered editor widget.
