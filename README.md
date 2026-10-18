# FlashVM workspace

Development version: **0.572**. [Русская версия](README.ru.md).

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
