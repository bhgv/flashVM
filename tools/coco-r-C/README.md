# Selected Coco/R C build sources

[Русский](README.ru.md) · [Provenance/update policy](../UPSTREAMS.md)

Source: https://github.com/bhgv/coco-r-C, master. Exact revision/files are in
[UPSTREAM.json](UPSTREAM.json). This snapshot contains the C generator, its own
.atg/frame sources, four C scanner/parser frames plus compiler.frm, and upstream
attribution. C++ backends, samples, binaries and documentation collections are
excluded. `upstream/` is not reformatted or locally patched.

From workspace root:

```sh
./build-workspace.sh --projects cocor --tests
```

Standalone: `cmake -S tools/coco-r-C -B build/cocor` followed by
`cmake --build build/cocor`. Native C compiler and CMake are enough for a build;
Python 3 is needed only for tests. The executable is build/cocor/cocor.
Use tools/coco-r-C/upstream as COCOR_ROOT and the built executable as
COCOR_EXECUTABLE for detached compiler projects. Output stays outside sources.

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
