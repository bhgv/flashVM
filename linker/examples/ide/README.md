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

## Ownership, errors and API stages

[main.c](main.c) separates setup, transformation and cleanup. Memory owns a
malloc buffer; capture copies the once-per-emission writer bytes before return;
publish checks both fwrite and fclose. compile owns three temporary streams and
transfers SWO only after a complete read. snapshot never retains its borrowed
FirModule pointer. Failures share cleanup; swl_destroy(NULL) is valid. A failed
output write can leave a partial destination file.

| API | Purpose and ownership |
| --- | --- |
| fir_temporary / fir_source | compile owns streams; numeric source becomes FIR |
| fir_run(FIR_OBJECT) | Verify/optimize per options, borrow before/after snapshots, emit SWO |
| swl_archive | Borrow input objects for the call; capture creates its own copy |
| swl_create / swl_add_archive | Linker context owns copies of accepted members |
| swl_add_rect / swl_place | Create the green shape and place it on stage |
| swl_emit | Borrow writer/userdata during complete SWF publication |
| swl_destroy | Free the context; first/second/library buffers are independent owners |

Exact declarations: [fir.h](../../../fir/v1/include/fir.h),
[swl.h](../../include/swl.h). Parsers are not reentrant; serialize each language's
calls. No library stage requires console control or launches subprocesses.
Exit 0 means SWF publication, 1 pipeline/I/O error, 2 invalid CLI arguments.
The test requires four snapshots and successful output; VM execution separately
checks 42/36. [Native/cross builds](../../../BUILDING.md) explains OS/CPU selection;
use the same toolchain for every linked library, never mix host/target archives.
