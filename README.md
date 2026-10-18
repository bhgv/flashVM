# FlashVM workspace

Development version: **0.574**. [Русская версия](README.ru.md).

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
| [flashVM/](flashVM/README.md) | VM source, bundled dependencies, examples, documentation, tools and build configuration |
| [flashVM/README.md](flashVM/README.md) | English VM guide and current capabilities |
| [flashVM/README.ru.md](flashVM/README.ru.md) | Russian VM guide |
| [flashVM/VERSION.txt](flashVM/VERSION.txt) | FlashVM development version |
| [view_docs.py](view_docs.py) | Workspace documentation entry point; currently delegates to FlashVM |

Future compiler components will live alongside `flashVM/`; they are not
implemented by this directory reorganization. Third-party dependencies retain
their own license notices inside the component.

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

The workspace will contain separate projects: FlashVM, ActionScript compilers,
a Limbo compiler, a SWF linker and, later, a Flash editor/IDE. Only FlashVM is
implemented here today. Each project owns its source, dependencies, build entry
point, version, documentation, examples and tests. No root build is required.
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
