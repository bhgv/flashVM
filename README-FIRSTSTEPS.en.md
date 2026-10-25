# First steps: source-owned build tools

[Русский](README-FIRSTSTEPS.md) · [Build reference](BUILDING.md) ·
[Source provenance](tools/UPSTREAMS.md)

Use Bash, CMake 3.10+, native C/C++ and Make/Ninja. Python 3 is used by the menu
and tests; Node is an optional test runtime for generated common-AS code only.
Termux prerequisites: `pkg install git clang make cmake python`.
Physical Android execution is not accepted by this series.

```bash
python3 build-menu.py
./build-workspace.sh
./build-workspace.sh --projects cocor,cocor-limbo,limbo-bootstrap --tests
./build-workspace.sh --projects compilers,fir,linker,symbols,disvm --tests
```

Choose one number/name, several separated by commas/spaces, or all. Individual
language projects are as1/as2/as3/limbo. Explicit tools and disvm have separate menu
entries. Builds are incremental; --build-root, --jobs and --config remain available.
Successful builds now install local packages under out/ or --out. No system
installation, cleanup, Git writes or scheduling is performed. Graphical builds
automatically download/build a pinned SDL2 SDK when no usable installed SDK exists.

Default Coco/R is source-built from tools/coco-r-C. The optional C/Limbo generators
are in tools/coco-r-limbo, including cocor.b/runtime fragments. Complex IDE code
uses the temporary source-built tools/limbo-bootstrap from the owner's experiment.
Both C and Limbo compiler versions are retained as references. Our Coco/R Limbo
compiler remains the priority; see compiler/limbo/OWN_COMPILER.md.

Explicit overrides remain: supply COCOR_ROOT+COCOR_EXECUTABLE together, optionally
LIMBO_EXECUTABLE and LIMBO_MODULE_DIR. Detached projects accept those SDK paths;
the root does not create implicit sibling dependencies inside components.

Outputs under build/workspace include vm/flashvm_reference_swf,
fir/{fir1_source,fir1,fir1_opt}, linker/swl, symbols/sblmap,
disvm/libdisvm_core.a and ide/{idecore.dis,idecore.sbl}. The Dis archive contains
the actual interpreter/JIT objects but is not yet a standalone host runner.
OS/kernel/scheduler/module-registration/native ABI work remains. It is not an
EMU_EXECUTABLE and no complete Dis/SWF runtime integration is claimed.

## IDE examples

```bash
# Production bytecode/debug data build from local sources requires no ready-made SDK.
./build-workspace.sh --projects ide
# Execution requires explicitly selected ABI-compatible native emu/compiler/headers.
python3 build-menu.py --projects ide --tests --demo history
```

--demo alone selects navigation; history selects Undo/Redo. A compiler and runtime
from different forks are not assumed frame/layout compatible. Supply the matching
compiler/header overrides when using another reference runtime. These examples
are console developer clients, not a complete graphical IDE. Historical full-VM
regression failures remain in ide/dis/SERIES_0_4_0.md.

## Our compiler: debug and two-way external modules

```bash
./build-workspace.sh --projects limbo,fir
mkdir -p build/output
export TMPDIR="$PWD/build/workspace/tmp"
build/workspace/fir/fir1_source limbo --input fir/v1/examples/limbo/scopes.b build/output/scopes.fir -g build/output/scopes.debug.json
build/workspace/limbo/frontend/limbo_bindings --format as-module compiler/limbo/examples/external-modules/arithmetic.m build/output/arithmetic-as.as --prefix arithmetic_as
build/workspace/limbo/frontend/limbo_bindings --format as-proxy compiler/limbo/examples/external-modules/arithmetic.m build/output/arithmetic-dis.as --prefix arithmetic_dis
```

One .m describes an external AS implementation callable from Limbo/Dis and an AS
caller of Dis. C99/Python/JSON outputs share declaration identities. The generated
adapters require a real checked host transport/loader/marshaler; source scaffolds
do not install that bridge. Full own AS/Limbo executable lowering remains open.
[Detailed example](compiler/limbo/examples/external-modules/README.md).

FIRDBG1 records source snapshots, pre-optimization SSA origins and unique lexical
owners. It is not final SWF/Dis/JIT PC relocation.
[Debug contract](fir/v1/doc/SOURCE_DEBUG.md).
For manual compiler invocations without /tmp, set writable TMPDIR explicitly.
The root script supplies build-owned temporary paths for its children only.

[Complete FIR → SWO → SWF pipeline](fir/v1/doc/PIPELINE.md),
[optional SDL2 player](flashVM/frontend/README.md),
[Dis update workflow](tools/UPSTREAMS.md),
[complementary runtime architecture](disVM/ARCHITECTURE.md).

## ReplaceAll — IDE 0.5.0

```sh
python3 build-menu.py --projects ide --demo replaceall
```

[Example / Пример](ide/dis/examples/replaceall/README.md).
[Interface migration / Обновление интерфейса](ide/dis/REPLACE_ALL.md): rebuild the module and every client with the same idecore.m.

[Real FIR diagnostics in Limbo IDE / Диагностика FIR в Limbo IDE](ide/dis/examples/compiler-location/README.md): `./build-workspace.sh --projects fir,ide --tests` with a matching Limbo/module/emu SDK.

## CMake 3.10.2 / Make / Inferno mk

```sh
# Полная сборка из исходников с CMake 3.10.2:
./build-workspace.sh
# Генератор yacc, временный Limbo и модуль IDE одним запуском:
./build-workspace.sh --projects iyacc,limbo-bootstrap,ide
# Прямая сборка компиляторов и линкера без CMake:
./build-workspace.sh --backend mk --projects compilers,linker,iyacc
./build-workspace.sh --backend make --projects compilers,linker
```

[Detailed build paths and limitations](BUILD-NATIVE.md).

List engines: `python3 build-menu.py --list-backends`.
Selection: `--backend cmake|meson|make|mk|auto`; the interactive menu also asks.
Meson/Make/mk support the native compiler/linker/tool subset. Full workspace
builds and shared tests use CMake 3.10.

### Selecting mk in the menu

`mk`, `make` and `meson` currently build only `compilers`, `as1`, `as2`,
`as3`, `limbo`, `linker`, `cocor`, `mk`, `iyacc`, and `limbo-bootstrap`. The full `all` selection,
workspace tests and the IDE example require `cmake` or `auto` (CMake 3.10+).
The menu validates compatibility before starting. Selecting `all` with `mk`
asks you to change the engine or explicitly choose components again; it never
silently drops components. Native engines offer tests for mk/iyacc/limbo-bootstrap; the IDE demo is unavailable.

```sh
python3 build-menu.py --projects compilers,linker,mk,iyacc --backend mk
```

## Build mk, iyacc and Limbo without emu or CMake

```sh
python3 build-menu.py --projects mk,iyacc,limbo-bootstrap --backend mk --tests
```

In the interactive menu select `14,15,16`, then `mk`. Products are
`build/workspace/mk/mk`, `build/workspace/iyacc/iyacc`, and
`build/workspace/limbo-bootstrap/limbo-bootstrap`. This is the temporary native
Limbo compiler from selected owner Inferno sources, not a Dis interpreter.
It is rebuilt from C with a parser freshly generated by the local iyacc.
Tests execute mk, regenerate the Limbo parser and check Dis, `.sbl` and legacy
C scaffold output. `make`, `meson` and `auto` also support this selection.

IDE, regression-client and console-example compilation no longer requires emu:

```sh
./build-workspace.sh --projects ide --tests
```

Without `EMU_EXECUTABLE`, compiler-product and Python-host tests run, while Dis
execution tests are unavailable. The menu does not offer an unavailable demo.
Explicit `--demo` still completes compilation, then reports `NOT RUN` and exits
with status 3; this is not successful IDE execution. Other selected workspace
components and their tests continue to build; failed checks still stop the build.

[Embedded Dis host requirements](disVM/HOST-REQUIREMENTS.md).

## Shared FlashVM/Dis host

```sh
./build-workspace.sh --projects disvm-host --tests
# Experimental REG isolation, not a completed parallel VM:
DVM_NATIVE_REGISTERS=ON ./build-workspace.sh --projects disvm --tests
```

Menu component `17` (`disvm-host`) is included in `all`.
[Four platforms, module sources and experiment limits](disVM/host/README.md).
[Blocking channels and waiter snapshots](flashVM/doc/CHANNEL_RENDEZVOUS.md).


## Installed packages and display profiles

Both hyphenated and underscored launchers work. Use --out to choose a local prefix,
--no-install to explicitly skip packaging, --vm-profiles minimal,agg for separate
VM variants. Graphical profiles automatically build the SDL2 frontend and missing
SDK; --no-player explicitly selects console-only builds. OS display/audio runtime
libraries are still needed to open physical devices.
The no-graphics-headless and agg-headless packages contain the console runner,
headers/libs and numeric movies that trace 123/42. agg-sdl2 additionally contains
flashvm_player and assigns the generic flashvm command to that actual player.
Use flashvm-console for numeric traces; run-demo.sh starts the windowed example.

Graphics/input/audio fixtures are under examples/player/animation_audio. Real
SWF6 video fixtures are supplied separately; FlashVM video playback remains
unsupported. IDE modules and debug symbols install to out/ide/modules with AS
and Limbo sources. vm/STATUS.txt explicitly describes the unfinished Dis host;
no silent emu requirement or successful dummy launcher is introduced.

See [editor channels](ide/EDITOR-CHANNELS.md), [AS](ide/as/README.md), and the
[native-tool tutorials](examples/firststeps/tools/README.md). Installed sibling
FIR/linker examples can be rebuilt after relocation; the acceptance script is
`python3 tools/test_installed_workspace.py --prefix out --scratch build/install-smoke`.

## Automatic Coco/R selection

Normal menu builds compile the bundled Coco/R sources and use matching frames;
no separate generator installation or SDK configuration is required. In auto
mode, stale/incomplete COCOR_ROOT/COCOR_EXECUTABLE overrides trigger a diagnostic
and automatic use of the bundled source build. All four C frame files are
validated before configuring compiler projects. The tools/coco-r-C wrapper path
is normalized to its upstream source root.

COCOR_SOURCE=bundled explicitly selects bundled sources. COCOR_SOURCE=external
is an optional strict policy for deliberate external SDK use; invalid paths
then stop the build. Complete valid overrides remain supported in auto mode.
SDK-selection regressions and Coco/R prose-spelling checks run automatically
with the generator's CTest suite when --tests is enabled. Executable names,
paths, variables and code examples retain their existing technical identifiers.

## Module versions and workspace owners / Модули и рабочее пространство

The Dis host package now contains a [versioned native module store](disVM/MODULE-STORE.md),
its public SDK and a fully commented C example. The IDE package contains
[ideworkspace](ide/WORKSPACE.md), debug symbols and a callable Limbo tutorial.
Both are built/installed by the existing disvm-host,ide selection. Their tests
exercise actual channels/native threads; full embedded Dis/IDE startup remains unfinished.

```sh
./build-workspace.sh --projects disvm-host,ide --tests
```

Документы, история и билеты компиляции принадлежат Limbo-сервисам. Хранилище
байтов модулей удерживает прежние версии для читателей и не заменяет парсер,
планировщик или GC. Устаревшие результаты сборки отвергаются по ревизии/билету.


## Runnable desktop and Dis checks / Запуск окна и проверок Dis

```sh
bash build-workspace.sh --projects vm,disvm --vm-profiles player --jobs 4
./out/run-checks.sh
./out/run-checks.sh --window
```

SDL2 SDK при необходимости собирается автоматически, без ручной установки.
`flashvm` в SDL2-пакете запускает настоящее окно, а `flashvm-console` — отдельную
консольную проверку. См. [запуск плеера](flashVM/frontend/RUNNING.md) и
[проверочные программы Dis](disVM/RUNNABLE-CHECKS.md). Проверки Dis/JIT исполнимы
нативно; полноценный хост исполнения произвольных Dis-модулей и IDE ещё не завершён.


## Interactive build and launch

Run `python3 build_menu.py`, enter `all`, then accept the defaults with Enter.
The menu explicitly selects the real window/input/audio AGG/JIT player and
installs binaries and examples. It also offers the windowed interpreter, both
variants, and console-only builds. Advanced menu settings cover jobs,
configuration, build/install paths, profiles and installation without CLI flags.
After success, choose checks, the graphical demo, both, or Enter to finish.
JIT and interpreter packages use separate directories and do not overwrite each
other. The full embedded Dis/IDE host remains in development.


## Native Dis executable and IDE services / Исполняемый Dis и сервисы IDE

`python3 build_menu.py` → `all` → Enter теперь также собирает `disvm-runtime`
для Linux AMD64. В конце меню: **4** — программа Dis с каналами, **5** —
консольные сервисы IDE. Бинарный файл, согласованный компилятор, `.dis`, `.sbl`
и примеры устанавливаются в `out/disvm-runtime/`.

Это реальное исполнение Limbo/Dis без внешнего `emu`. Используется отдельный
экспериментальный NC64-формат; старые `.dis` не смешиваются с нативными модулями.
Графический редактор и полный набор Sys ещё не подключены. Подробности и
точные границы — в [руководстве рантайма](disVM/runtime/README.md).


## Интерактивный редактор на Dis / Interactive Dis editor

В `build_menu.py` выберите `all`, затем в меню запуска **5**. Доступны `help`,
`show`, `set`, `append`, `insert`, `delete`, `find`, `replaceall`, `undo`, `redo`,
`load`, `save`, `quit`. `complete as ...` / `complete limbo ...` показывают
декларации, `hint LANGUAGE NAME ARGUMENT` — справку по параметрам. Всё выполняется
на собственном Dis с настоящими каналами и реальной файловой записью.
Пункт **6** сохраняет прежнюю автоматическую демонстрацию сервисов.
[Подробное руководство и ограничения](ide/dis/examples/editor-console/README.md).
