# SWL linker 0.3.14

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

C99 sources use four-space indentation, expanded blocks and visible error
handling. The component-owned [.clang-format](.clang-format) survives detached
use. Generated Coco/R code and upstream libraries are excluded.

[Build for the host or another OS/CPU](../BUILDING.md).

[Object/library inspection](INSPECTION.md).

Stage policy is also available from the CLI:

```sh
build/linker/swl link movie.swf --size 640 480 --fps 30 library.swa
build/linker/swl inspect library.swa
```

Size is 1..3276 pixels per axis; rate is an integer 1..255 frames/second.
Defaults remain 550×400 at 12 fps. Options can precede or follow inputs;
repeated stage options use the last value. Invalid/missing stage values return
usage status 2 before opening inputs or output. Resource operand slots are never
interpreted as options: a file literally named `--extensions` cannot enable
FlashVM requirements. The standalone C API continues to use `SwlOptions`.

[C inspection tutorial](examples/inspect/README.md).

[Example catalog](examples/README.md).

Standard AVM1 BitXor (0x62) is accepted alongside BitAnd/BitOr for signed32 FIR output; SWO1/SWF8 formats are unchanged. Стандартный BitXor принимается без изменения форматов.

SWL 0.3.14 accepts standard AVM1 Not (0x12) for strict FIR bool.not. SWO1/SWF8 formats are unchanged; validation preserves exact action bytes. / Стандартный Not принимается без изменения форматов; байты действий сохраняются.
