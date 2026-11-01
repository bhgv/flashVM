# Scalar compiler, field-witness and interoperability checkpoint

[Русский](DIS64-SCALAR-COMPILER.ru.md) · [Synchronization](SYNC.md) ·
[Upstream cross-reader contract](upstream/doc/dis64-external-reader.md)

The selected Inferno sources are pinned to
`4fd422043088d9ba8bf697db89d2069ec40ef9f4`. This imports INFERNO-0052..0054
and the reverse FLASHVM-0050 review/test. The native Limbo bootstrap now builds
its restricted `-6` producer. Our Coco/R Limbo → FIR compiler remains primary;
the historical parser is confined to the explicit temporary bootstrap.

## Compile and inspect

The upstream example has one pointer-free, parameterless `init()`, integer/Big/
Real scalar storage and a self-checking 4097-iteration loop. It imports no Sys,
Draw or other managed module. Compile and inspect using built products:

```sh
out/limbo-bootstrap/limbo-bootstrap -6 -o loop.dis64 \
    examples/firststeps/tools/limbo-scalar/loop.b
out/disvm/dis64inspect loop.dis64 --pilot --rewrite checked.dis64
```

Use actual installed paths for the selected package. The reader admits the
existing scalar pilot while reporting `execution_supported=false`; this is not
permission to launch arbitrary Dis64 through the default runtime. File
CALL/MCALL, imports, managed values and a full Limbo shell remain closed.

The compiler rebases ordinary direct locals by 32 bytes and moves legacy scalar
scratch bytes to a separate aligned 16-byte frame tail. The reserved return-word
access is forbidden. Scratch aliases stay aliases; map geometry does not turn
these bytes into compiler-declared native arguments. Unsupported source shapes
fail without publishing a partial image. Legacy Dis output stays unchanged.

## Explicit optional CTest input

```sh
cmake -S disVM -B build/dis-review \
    -DDVM_NATIVE64_LIMBO_EXECUTABLE=/absolute/build/limbo-bootstrap
cmake --build build/dis-review
ctest --test-dir build/dis-review --output-on-failure \
    -R 'disvm_native64_field_|disvm_dis64_external_reader'
```

The compiler input is explicit and optional. No hidden sibling compiler or
Python deployment dependency is introduced. Five field tests execute production
metadata, witness and Math code: normal plus kind/owner/generation/field contract
removals. Native Linux x64/ARM64 host guards remain explicit; this does not run a
cross target automatically.

The external-reader test checks 64 local layouts, exact image rewrites, unchanged
ordinary Dis and 64 managed-map refusals preserving the rewrite destination.
For independent producer comparison, supply `--other-limbo` as documented in the
upstream guide. Its optional complete-emulator path executed 128 successful
processes plus two active wrong-answer/error-retirement controls on native x64.
This is separate from header fixtures and does not claim physical ARM64/RV64
acceptance.

## Private field witness scope

Compiler metadata distinguishes Big from Real and padding using exact source
ordinals, kinds and widths. The witness binds it to an exact generated callable,
retained Module/Type, Modlink instance and current-caller activation lease. The
normal gate passed 351 predicates per compatible WORD model; all four removal
controls reached semantic failures in both models.

This accepts a synchronous trusted C profile only. Immutable generated/export
storage and constructor pairing remain obligations. No arbitrary C table is
authenticated as compiler output. The current-caller hold cannot cross an async
job/channel wait. Detached process lifetime, moving/suspended callers, managed
argument/result graphs and prior optimized UTF/unwind ASan findings remain open.
No new section, feature, scalar-kind or opcode IDs are assigned.
