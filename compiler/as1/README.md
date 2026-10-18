# AS1

Version 0.1.0. [Русский](README.ru.md).

Independent C99 compiler project. The current implementation is a syntax
frontend: [grammar, build and limitations](frontend/README.md). Semantic
analysis, AST construction, bytecode and SWF emission are not implemented yet.

Configure this directory with the same explicit tool paths as in the frontend guide:

```sh
cmake -S . -B build \
  -DCOCOR_ROOT=/path/to/coco-r-C \
  -DCOCOR_EXECUTABLE=/path/to/coco-r-C/cocor
cmake --build build
ctest --test-dir build --output-on-failure
```
