# Reproducing the native64 ownership review

[Русский](NATIVE64-REVIEW.ru.md)

The selected Inferno source contains a separate, experimental WORD32/native64
ABI path. It is distinct from the runnable NC64 IDE runtime. Importing these
sources does not enable a new Dis64 file profile or switch SDK defaults.

## Two explicit review gates

With a GNU/Clang Linux64 toolchain supporting strict UBSan, configure the existing
runtime build with its normal editor/compiler options and additionally:

```sh
cmake -S disVM/runtime -B build/editor-review \
    -DDVM_RUNTIME_TESTS=ON -DDVM_RUNTIME_NATIVE64_REVIEW_TESTS=ON
cmake --build build/editor-review --parallel 4
ctest --test-dir build/editor-review -R disvm_native64_review --output-on-failure
```

`disvm_native64_review_module_sets` exercises actual generated declarations,
module/Type/link construction, fault retirement and atomic publication. The
independent FlashVM ordering oracle checks 72 request/removal permutations per
compatible model, including actual calls through surviving modules and borrowed
output lifetimes. The gate also rejects legacy/packed host models and runs active
publication/retirement removal controls.

`disvm_native64_review_typed_sys` exercises owned argument snapshots, complete
map8 roots, the real formatter, bounded output growth and four trusted Sys C
exports. Actual collector, heap, conversion and emu/os protection bodies run with
bounded native scheduler/output/allocation services. Compiled removal controls
must fail their active ownership or scheduler predicates. Native write bytes can
already have escaped when late scalar publication fails: this status is never a
promise that retrying is safe.

Both gates use the source-built `limbo-bootstrap` only to produce native64 C
metadata. They do not execute its NC64 output as Dis64. The optional switch adds
development tests; sanitizers are not a dependency of the installed player.

## Broader independent integration

```sh
NATIVE64_LIMBO=/absolute/path/to/fresh/limbo \
NATIVE64_MATH_LIBDIR=/absolute/path/to/source-built/native/lib \
NATIVE64_CFLAGS='-O3 -g -DNDEBUG -fno-strict-aliasing -fsanitize=undefined -fno-sanitize-recover=all' \
python3 disVM/upstream/utils/tests/native64-layout.py
```

The explicit library directory must contain owning classic-mk `lib9.a` and
`libmath.a`; without it complete Math registration is reported as skipped. The
full gate includes all 68 Math exports, prior layout/GC/result tests, typed Sys
and module sets. Prebuilt numeric archives are not whole-library sanitizer
coverage. These checks are host fixtures, not physical ARM64/RV64 execution.

## Agreement and limits

The journals live in the owning Inferno experiment branch:
[Inferno](https://github.com/bhgv/Inferno-OS-bhgv/blob/experiment/doc/dis64-inferno-journal.md)
and [FlashVM](https://github.com/bhgv/Inferno-OS-bhgv/blob/experiment/doc/dis64-flashvm-journal.md).
The four generated packed Sys variadic refusals and full-startup guard remain.
Frame64/map8, the default 43-op np0 admission, `execution_supported=false` and
NC64/SDK defaults remain unchanged. Private typed C tags are not wire kind IDs.
Prior optimized UTF and injected-unwind ASan evidence remains unresolved.
Atomic module graph publication does not run or roll back initializer effects.

CTest places private generated review files under the build directory via
`NATIVE64_TMPDIR`; direct driver invocations may set the same environment variable
to an existing writable directory. It changes no source selection or ABI.
