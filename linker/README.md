# SWL linker 0.3.2

Working independent C99 component. FIR compiles into `.swo`; SWL combines objects
and `.swa` libraries into SWF8. The compiler no longer creates SWF containers.
Initializers execute in input/member order. Module names must be unique;
SSA storage is prefixed by its module. Libraries include all members, without
implicit search paths or lazy function resolution. FIR cross-module calls and
function imports/exports are not implemented yet.

```sh
cmake -S linker -B build/linker
cmake --build build/linker
ctest --test-dir build/linker --output-on-failure
build/fir1/fir1 object input.fir input.swo
build/linker/swl archive library.swa first.swo second.swo
build/linker/swl link movie.swf library.swa third.swo
build/linker/swl --help
```

API: [include/swl.h](include/swl.h). `swl_create`/`swl_destroy` own a context;
`swl_add_object`/`swl_add_archive` copy and validate inputs; `swl_emit` calls the
provided writer after complete assembly. `swl_archive` creates a library.
The library prints nothing, opens no paths and never exits the process.
Failed additions roll back; contexts remain reusable. Distinct contexts are
independent; do not mutate one context concurrently. FlashVM requirements are
rejected by default; opt in with `SwlOptions.allow_flashvm_extensions` or
`--extensions`.

[Formats](FORMAT.md). C99, without FIR, Coco/R or VM dependencies. Detached
copies retain the root MIT LICENSE. CLI statuses: 0 success, 1 input/link/write
failure, 2 invalid arguments. Inputs/output are bounded by 64 MiB, at most 1024
objects. Validation failures preserve existing output; a write failure itself
may leave a partial output file.

[Resources, SVF1 and scene](ASSETS.md).

[Verification and limitations](TESTING.md).
