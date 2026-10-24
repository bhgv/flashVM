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
