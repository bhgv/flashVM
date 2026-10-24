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
