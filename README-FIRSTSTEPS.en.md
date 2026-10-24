# First steps

[Подробное руководство на русском](README-FIRSTSTEPS.md) · [Build reference](BUILDING.md)

Run from the repository root with Bash, CMake 3.16+, a native C/C++ compiler,
Make/Ninja and Python 3 for tests. The optional root script preserves independent
component builds. Termux prerequisites: `pkg install git clang make cmake python`.
Physical Android was not tested in this series.

Build the external generator once:

```bash
git clone https://github.com/bhgv/coco-r-C.git ../coco-r-C
make -C ../coco-r-C/sources -f unix.mk
export COCOR_ROOT="$(cd ../coco-r-C && pwd)"
export COCOR_EXECUTABLE="$COCOR_ROOT/sources/cocor"
```

For IDE builds supply `LIMBO_EXECUTABLE` pointing to your native Limbo compiler.
For IDE tests/demo also supply native `EMU_EXECUTABLE` and `LIMBO_MODULE_DIR`
containing sys.m. See [SDK and reference execution](ide/dis/README.md) and
[reference runtime](ide/dis/REFERENCE_RUNTIME.md). Tools must match the host OS
and architecture. External Inferno/Dis is not downloaded or embedded by this script.

```bash
./build-workspace.sh --help
./build-workspace.sh --jobs 2
./build-workspace.sh --jobs 2 --tests
./build-workspace.sh --projects vm,linker
./build-workspace.sh --projects compilers,fir
./build-workspace.sh --projects ide --demo
```

Repeat commands for incremental builds. Default outputs are in build/workspace,
with one directory per selected project and temporary files in tmp. Use
`--build-root build/another` when changing SDK/compiler; `--config Debug` changes
configuration. SDK checks precede builds; failures stop subsequent work and retain
partial outputs. No installation, cleanup, Git operation or scheduling occurs.

Outputs include vm/flashvm_reference_swf, fir/{fir1_source,fir1,fir1_opt},
linker/swl, symbols/sblmap and ide/{idecore.dis,idecore.sbl}. Syntax drivers are under
compilers/LANGUAGE/frontend/. Multi-config generators may add configuration folders.
The Russian guide includes a complete source → FIR → SWO → SWF command example;
see also the [pipeline tutorial](fir/v1/doc/PIPELINE.md).

`--demo` runs the documented [Limbo navigation client](ide/dis/examples/navigation/README.md)
in both reference emulator modes; expect `PASS idecore navigation 16 checks`.
The library core has no standalone init entry point. The graphical IDE and embedded
Dis/JIT remain under development. Reference modes do not prove new native JIT entry.
Compiler syntax recognition and numeric lowering are bounded, not complete languages.
The optional windowed [SDL2 player](flashVM/frontend/README.md) has a separate SDK/build.

## Component menu

Run `python3 build-menu.py` in a terminal. Select one number, several numbers
such as `1,4,6`, names such as `vm linker ide`, or `all`. Enter `q` to exit.
The menu offers tests and, when IDE is selected, its console example.
`compilers` groups all four syntax frontends. The shared Bash builder handles
SDK checks and builds; Python is host tooling only.

For unattended use:

```bash
python3 build-menu.py --projects linker --tests
python3 build-menu.py --projects vm,linker --jobs 4
python3 build-menu.py --projects ide --demo
python3 build-menu.py --projects all --dry-run
```

`--build-root` and `--config` also work. SDK variables are inherited.
Dry-run prints the command without executing it; normal execution streams build
output and returns the builder exit status. Ctrl-C exits with status 130.

Select individual frontends with `--projects as1` or `--projects as2,limbo`.
`all` uses the grouped compiler build without duplicate individual builds.

## History and source diagnostics

Run `python3 build-menu.py --projects ide --tests --demo history` for the new
channel-only Undo/Redo example. `--demo` alone still selects navigation.
After building FIR, run `build/workspace/fir/fir1_diagnostic_example` for the
commented C99 diagnostic client. `ide/build.py --structured-diagnostics` opts in
to validated byte spans and Unicode positions from FIR 0.5.4. Match snapshot
SHA-256 and live revision before applying late diagnostics.
[History](ide/dis/HISTORY.md) · [Diagnostic schema](fir/v1/doc/SOURCE_DIAGNOSTICS.md).

For manual compiler/example invocations in a host without /tmp, export
`TMPDIR="$PWD/build/workspace/tmp"` and create that directory first. The root
script sets build-owned temporary paths for its own children only.
