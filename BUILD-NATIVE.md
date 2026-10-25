# CMake 3.10, Meson, Make and Inferno mk build paths

[Русский](BUILD-NATIVE.ru.md) · [First steps](README-FIRSTSTEPS.en.md) ·
[mk](tools/mk/README.md) · [iyacc](tools/iyacc/README.md)

The complete workspace builds with **CMake/CTest 3.10.2**, tested with the actual
official Linux AMD64 binaries. No CMake upgrade is required. Configure uses a
positional source path from the build directory; native builds use -- -jN;
CTest runs in its build directory with -N zero-test admission. All owned component
minimums are 3.10, with old FindPythonInterp only on CMake versions lacking
FindPython3. FIR/SBL object files are included explicitly and IDE admission uses
bounded reads instead of file(SIZE). Modern CMake remains supported.

```sh
./build-workspace.sh
python3 build-menu.py
```

The default builds mk and iyacc from the owner's selected sources plus the eight
workspace products. Explicit selectors remain available. When the source Limbo
bootstrap is needed, mk/iyacc are built automatically and its parser regenerated.
Builds never download source, tool binaries or SDKs. Python 3 is needed for tests
and the menu, not for deployed IDE execution. A matching external VM is still
needed for IDE runtime tests/demo; the DisVM core archive is not a host runner.

## Direct native fallback

```sh
./build-workspace.sh --backend mk --projects compilers,linker,iyacc
./build-workspace.sh --backend make --projects compilers,linker
make compilers linker
# Or bootstrap only mk, then use its actual mk dialect:
sh tools/mk/bootstrap.sh build/mk
build/mk/mk -f mkfile compilers linker
```

These targets compile directly using CC/ar, without CMake delegation. Supported:
mk, iyacc, limbo-bootstrap, `cocor`, as1/as2/as3, limbo (parse/tree/interface/bindings) and compilers,
plus the SWO/SWA/SWF linker. Owned frontend parsers use bundled source-built Coco/R; the temporary
legacy Limbo bootstrap uses local iyacc. Native --tests is supported when every
selected component is mk, iyacc or limbo-bootstrap. VM/FIR/SBL/IDE/DisVM and
--demo remain unsupported by native backends; use CMake 3.10 for those builds. Meson 0.54+ with Ninja supports the same native subset through real run targets; it does not invoke CMake. Native recipes always rebuild;
CMake builds retain incremental dependency tracking. Native defaults are CC=cc,
AR=ar, BUILD_ROOT=build/native (root launcher defaults to build/workspace).

## Validation and limits

Real CMake/CTest 3.10.2: compilers 40/40, FIR 14/14, linker 3/3, symbols 3/3,
DisVM 15/15, Coco/R and native-bootstrap product checks; FlashVM library and
reference SWF executable built. Production IDE compiled with the regenerated
bootstrap; explicit reference SDK Limbo tests 25/25 passed. Actual mk built all
four frontends and linker under paths containing spaces; native generated C/Python
bindings, all three AS adapter parses and 199 linker checks passed. GNU make
built the native linker. The mk smoke test covers wildcard recipes, timestamps
and child failure; iyacc generated real limbo.y with identical token header.

The unchanged Python host IDE suite reported two timeout-cleanup subcase failures
(link/run) in this workspace, matching the earlier recorded cleanup limitation.
They are not converted into successes or attributed to CMake; --tests propagates
that failure. Full FlashVM runtime regression and physical Android/ARM/RISC-V
acceptance were not rerun. Upstream mk/iyacc warnings and legacy temporary-file
behavior are preserved; no confirmed upstream interpreter/JIT fix is part of this
build series. Only selected tools and dependencies are imported.

This series replaces the proposed 0.1530 tool-version rejection patch. Its base
is published 0.1529. Apply this patch on that base; do not apply the older 0.1530
patch as a prerequisite.

## Engine selection / Выбор движка

```sh
python3 build-menu.py --list-backends
python3 build-menu.py --projects compilers,linker --backend meson
python3 build-menu.py --projects all --backend cmake
python3 build-menu.py --projects compilers,linker --backend make
python3 build-menu.py --projects compilers,linker --backend mk
python3 build-menu.py --projects all --backend auto
```

CMake/Meson/Make paths honor CMAKE/MESON/MAKE; Meson also honors NINJA.
Interactive mode asks for cmake, meson, make, mk or auto and shows executable
presence. mk can bootstrap from its selected sources. Auto chooses CMake 3.10+
first; for the supported native subset, then Meson/Ninja, Make and source-built
mk. It rejects an unsupported full selection instead of omitting components.
Explicit choices never silently switch engines. `--dry-run` prints the command
without requiring the selected optional engine. Meson 1.5.2/Ninja 1.11.1 actually
built all four frontends and linker in this series; auto selected real CMake
3.10.2 for linker tests and Make when CMake/Meson paths were absent.

[Source-only mk/iyacc/Limbo builds and emu-free IDE compilation](README-FIRSTSTEPS.en.md)
