# FlashVM workspace

Development version: **0.799**. [Русская версия](README.ru.md).

## Browse documentation

From this repository's root:

```sh
python -m pip install -r flashVM/doku_viewer/requirements.txt
python view_docs.py
```

The root launcher currently opens the [FlashVM documentation](flashVM/README.md).
It delegates to `flashVM/view_docs.py`, preserving `--port` and `--no-browser`.
Python 2.7.9 / Python 3 compatibility is retained; this layout change was tested
with Python 3, not presented as a fresh Python 2.7.9 runtime validation.

## Layout

| Path | Purpose |
| --- | --- |
| [LICENSE](LICENSE) | Shared MIT license for first-party workspace code, copyright bhgv |
| [compiler/](compiler/README.md) | Independent AS1, AS2, AS3 and Limbo C99 syntax projects |
| [linker/](linker/README.md) | Independent C99 SWO/SWA and resource linker to SWF8 |
| [flashVM/](flashVM/README.md) | VM source, bundled dependencies, examples, documentation, tools and build configuration |
| [flashVM/README.md](flashVM/README.md) | English VM guide and current capabilities |
| [flashVM/README.ru.md](flashVM/README.ru.md) | Russian VM guide |
| [flashVM/VERSION.txt](flashVM/VERSION.txt) | FlashVM development version |
| [view_docs.py](view_docs.py) | Workspace documentation entry point; currently delegates to FlashVM |

Syntax frontends live in [compiler/](compiler/README.md). The separate
[FIR toolchain](fir/v1/README.md) includes numeric source adapters, verification,
optimization and SWO object emission. [SWL](linker/README.md) links objects,
whole-member libraries, shapes, JPEG and PCM WAV resources into SWF8. This is a
bounded working pipeline, not complete AS/Limbo language support. Third-party
dependencies retain their own license notices inside each component.

## Build FlashVM

```sh
cmake -S flashVM -B flashVM/build
cmake --build flashVM/build
ctest --test-dir flashVM/build --output-on-failure
```

Alternatively, `cd flashVM` and use the component's existing commands. Its DEFS,
public headers, source layout and internal relative paths remain project-local.
See [build definitions](flashVM/DEFS.txt), the [roadmap](flashVM/doc/ROADMAP.md)
and [release acceptance](flashVM/doc/RELEASE_ACCEPTANCE.md). Moving files does
not close any compatibility or platform acceptance stage.

## Independent projects

FlashVM, language syntax frontends, FIR/compiler/optimizer and SWL are separate
working projects. A Flash source/image editor remains future work. Each project
owns its source, dependencies, build entry point, version, documentation,
examples and tests. No root build is required.
Future workspace automation may orchestrate builds, but must remain optional.

A project must remain buildable after copying its directory to another location
or repository, including under a different directory name. Do not include sibling
source trees through implicit `../` paths. Required dependencies must be documented
and supplied as explicit SDK/library/tool paths or bundled where permitted.
Compilers, linker and VM should exchange documented artifacts and public APIs.
The editor consumes those tools through explicit interfaces.

To separate FlashVM, copy `flashVM/` and place the shared root `LICENSE` in the
new distribution root (beside that folder), preserving bundled license notices.
Run CMake and `view_docs.py` inside the copied project. The component viewer serves
only that project; the workspace launcher explicitly enables workspace navigation.
The Git-history archive utility requires the original Git repository and is not
needed to build or run the separated VM. Repository history and workspace helper
files are not build dependencies.

## Compiler and linker projects

[compiler/](compiler/README.md) contains independently buildable C99 syntax
frontends for [AS1](compiler/as1/frontend/README.md),
[AS2](compiler/as2/frontend/README.md), [AS3](compiler/as3/frontend/README.md)
and [Limbo](compiler/limbo/frontend/README.md). Each uses a readable authoritative
`.atg`, external bhgv/coco-r-C, a C API, a console driver and syntax tests.
Component versions are independent; consult each project's VERSION or README.

The separate numeric [source adapters](fir/v1/doc/SOURCE_SYNTAX.md) emit FIR.
FIR verification/optimization precedes SWO generation; the FIR compiler never
creates a SWF container. [SWL](linker/README.md) combines SWO/SWA and resources,
with explicit FlashVM extension opt-in. Broad syntax recognition is not full
semantic lowering: AS3 ABC and FIR cross-module function relocations remain
unimplemented. [Pipeline help](fir/v1/doc/PIPELINE.md),
[SWL examples](linker/examples/README.md),
[typed C inspection API](linker/INSPECTION.md).

## Desktop SWF player

The optional [FlashVM desktop frontend](flashVM/frontend/README.md) has Linux,
Windows and macOS launchers, `-h`/`--help`, interpreter/JIT, graphics quality,
absolute FPS and percentage speed options. It requires an SDL2 development SDK:

```sh
cmake -S flashVM -B flashVM/build-desktop -DFVM_BUILD_DESKTOP_FRONTEND=ON
cmake --build flashVM/build-desktop --target flashvm_player --parallel
./flashVM/build-desktop/frontend/flashvm_player --help
```

## FIR v1

[Format, C99/Coco-R tools and tutorials](fir/v1/README.md).

[ATG layout and style checks](tools/README.md).

[Native and cross builds for every component](BUILDING.md),
[compiler example catalog](compiler/examples.md).

[AS1/AS2/AS3/Limbo/FIR parsers: series 0.6.0](compiler/PARSER_COVERAGE.md).
