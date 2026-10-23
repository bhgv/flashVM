# C89 tiny cubic extrema

This executable builds a native cubic with Y controls 0,12,-12,0, checks analytic extrema ±2√3, scales the path by 2^-600 and repeats the check after normalization. It exercises public path geometry, not new cubic support in managed MovieClip. No graphics, fonts, VM, GC, device or external SDK is needed. Ordinary failures return 1; success prints `cubic extrema verified`. Pixels are not tested.

## Build / Сборка

From the standalone FlashVM component / Из отдельного компонента FlashVM:

```sh
cmake -S . -B build-cubic -DFVM_DEFS_FILE="$PWD/cmake/profiles/minimal.defs"
cmake --build build-cubic --target flashvm_examples_embedding_cubic_extrema_main
ctest --test-dir build-cubic -R '^flashvm_examples_embedding_cubic_extrema_main$' --output-on-failure
```

## Individual APIs, ownership and execution

README.md documents every demonstrated C API. The caller owns the path until destroy. Command/matrix/bounds live on the stack; there are no VM roots or GC interactions. Concurrent path mutation or destruction during reads is forbidden. Operations are synchronous without callbacks or device work.

| API | Purpose, arguments, errors and ownership |
| --- | --- |
| [`fvm_path_create`](../../../include/flashvm/path.h#L50) | Acquire a native owner through out; check [FVM_OK](../../../include/flashvm/flashvm.h#L42) before use, with [FVM_INVALID_ARGUMENT](../../../include/flashvm/flashvm.h#L45)/FVM_OUT_OF_MEMORY errors. Release the returned owner exactly once through destroy. |
| [`fvm_path_add`](../../../include/flashvm/path.h#L54) | Borrow path and copy command; MOVE starts a contour, CUBIC contains two controls and an endpoint. Invalid commands/coordinates yield [FVM_INVALID_ARGUMENT](../../../include/flashvm/flashvm.h#L45); failed storage yields [FVM_OUT_OF_MEMORY](../../../include/flashvm/flashvm.h#L44). No command pointer is retained. |
| [`fvm_path_bounds`](../../../include/flashvm/path.h#L71) | Borrow an immutable path and publish four caller-owned scalars on [FVM_OK](../../../include/flashvm/flashvm.h#L42); invalid path/out yields [FVM_INVALID_ARGUMENT](../../../include/flashvm/flashvm.h#L45). No allocation, callback, GC or owner transfer. |
| [`fvm_path_transform`](../../../include/flashvm/path.h#L65) | Synchronously edit the native path using a borrowed matrix; invalid matrices/coordinates outside ±1,000,000 yield [FVM_INVALID_ARGUMENT](../../../include/flashvm/flashvm.h#L45) and copy failures yield [FVM_OUT_OF_MEMORY](../../../include/flashvm/flashvm.h#L44). Failure preserves the old path. |
| [`fvm_path_destroy`](../../../include/flashvm/path.h#L52) | Release the native owner and invalidate path; null is accepted and no status is returned. Reading, editing or destroying that handle again is forbidden. |

[Contract and limits](../../../doc/CUBIC_EXTREMA_SCALING.md). [main.c](main.c), [example_constants.h](example_constants.h). Ordinary binary arithmetic without FTZ/fast-math is required; physical-platform and complete SWF ≤ 8 acceptance are not claimed.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_INVALID_ARGUMENT` | enum constant | [include/flashvm/flashvm.h:45](../../../include/flashvm/flashvm.h#L45) |
| `FVM_OK` | enum constant | [include/flashvm/flashvm.h:42](../../../include/flashvm/flashvm.h#L42) |
| `FVM_OUT_OF_MEMORY` | enum constant | [include/flashvm/flashvm.h:44](../../../include/flashvm/flashvm.h#L44) |
| `fvm_path_add` | function | [include/flashvm/path.h:54](../../../include/flashvm/path.h#L54) |
| `fvm_path_bounds` | function | [include/flashvm/path.h:71](../../../include/flashvm/path.h#L71) |
| `fvm_path_create` | function | [include/flashvm/path.h:50](../../../include/flashvm/path.h#L50) |
| `fvm_path_destroy` | function | [include/flashvm/path.h:52](../../../include/flashvm/path.h#L52) |
| `fvm_path_transform` | function | [include/flashvm/path.h:65](../../../include/flashvm/path.h#L65) |
<!-- source-links:end -->
