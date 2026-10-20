# IDE embedding example

[main.c](main.c) links compilers, optimizer and linker into one C process. It
compiles AS2 and Limbo with distinct module names, receives before/after FIR
snapshots, creates `.swo`, packages a `.swa` library, adds a shape and emits SWF.
With FRONTENDS_ROOT it also links all four broader syntax frontends into the
same process. The example invokes no child processes or shell commands.

```sh
cmake -S linker/examples/ide -B build/ide \
  -DFIR_ROOT="$PWD/fir/v1" -DSWL_ROOT="$PWD/linker" \
  -DFRONTENDS_ROOT="$PWD/compiler" \
  -DCOCOR_ROOT="$COCO_DIR" -DCOCOR_EXECUTABLE="$COCO_DIR/sources/cocor"
cmake --build build/ide --target ide_pipeline
build/ide/ide_pipeline build/ide/movie.swf
```

The SWF traces 42 then 36; a green shape is placed at (20,30), depth 1. This is
a control example, not a finished IDE. Snapshot modules are borrowed only for
the callback duration. Component roots are explicit and may be detached/renamed;
external Coco/R and the root MIT LICENSE are required.
