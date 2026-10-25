# Transactional path fragments without repeated prefix copying

[Русский](PATH_APPEND_GROWTH.ru.md) · [Public implementation](../src/capi/path_c.cpp) ·
[Independent allocation/alias/error oracle](../tests/path_append_transaction.cpp).

[`fvm_path_append`](../include/flashvm/path.h#L91) formerly cloned the complete destination for every appended
glyph/fragment. Building a long TextField path therefore repeatedly allocated
and copied the same prefix. The clipboard boundary test exposed this with a
64 KiB long line: the old path builder exceeded the existing editor deadline.

The operation now stages only transformed source commands, including when
source==destination. Every transform and potentially failing allocation occurs
before destination command writes. A geometric destination reservation prevents
reallocation on every small append. [FVM_PathCommand](../include/flashvm/path.h#L30) contains scalar POD only;
reserved pushes cannot allocate or throw. Final contour-open state is published
only after all commands are appended. Empty sources preserve the previous state.

The public command/coordinate limits, geometry, C89 API, C++98 implementation,
self-alias behavior and failure atomicity are unchanged. Capacity is private;
no pointer, container ABI, callback or managed-GC contract is added.

The regression injects staging/reservation allocation failures, rejects a late
transformed point outside bounds, verifies original commands/open state and
checks transformed self-append. A 1024-fragment oracle bounds cumulative
allocation bytes linearly relative to output, without a wall-clock assertion.
The old implementation fails that budget; the new implementation passes.
125 selected backend-independent path/raster checks pass. Real graphical editor
and separate UBSan acceptance are recorded with the workspace series.
All nine roadmap stages and general reference/device acceptance remain open.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_path_append` | function | [include/flashvm/path.h:91](../include/flashvm/path.h#L91) |
| `FVM_PathCommand` | struct | [include/flashvm/path.h:30](../include/flashvm/path.h#L30) |
<!-- source-links:end -->
