# Resource through 9P2000

Runnable C89 tutorial reads `/1.1/path` using standard binary
version/attach/walk/open/read frames. No VM, SDL, socket or mount is needed.

```sh
cmake -S flashVM -B build-9p -DFVM_BUILD_EXAMPLES=ON
cmake --build build-9p --target flashvm_examples_embedding_resource_9p_main
./build-9p/flashvm_examples_embedding_resource_9p_main
```

Inside the component use `cmake -S .`. Requires C89 and CMake and links only
flashvm_srecord. Expected: `9p resource path: /scene/root`. The source contains
annotated wire fixtures; `1.1` is valid only for this fresh registration. Real
apps enumerate names. Host grants authority beforehand; uname grants none.
FS borrows dispatcher, server borrows FS and owns fids. Cleanup is server, FS,
dispatcher, including errors. A production host waits for callbacks and retries
BUSY destroy. This example reads path metadata and never invokes its registration
callback. It is not a TCP listener, authentication, OS mount or full Inferno tree.

| API | Ownership / execution |
| --- | --- |
| [`fvm_rdispatch_create`](../../../include/flashvm/rdispatch.h#L95) | Own one single-owner dispatcher. [doc/RESOURCE_DISPATCH.md](../../../doc/RESOURCE_DISPATCH.md). |
| [`fvm_rdispatch_register`](../../../include/flashvm/rdispatch.h#L99) | Copy resource metadata, borrowing its callback userdata. [doc/RESOURCE_DISPATCH.md](../../../doc/RESOURCE_DISPATCH.md). |
| [`fvm_rdispatch_grant`](../../../include/flashvm/rdispatch.h#L108) | Owner grants READ to the trusted local session. [doc/RESOURCE_DISPATCH.md](../../../doc/RESOURCE_DISPATCH.md). |
| [`fvm_rfs_create`](../../../include/flashvm/rfs.h#L49) | Allocate FS with exclusive session IDs; borrow dispatcher. [doc/RESOURCE_FILES.md](../../../doc/RESOURCE_FILES.md). |
| [`fvm_r9p_create`](../../../include/flashvm/r9p.h#L20) | Allocate fixed connection storage borrowing the FS. [doc/RESOURCE_9P.md](../../../doc/RESOURCE_9P.md). |
| [`fvm_r9p_request`](../../../include/flashvm/r9p.h#L28) | Process one frame into owned output without running VM work. [doc/RESOURCE_9P.md](../../../doc/RESOURCE_9P.md). |
| [`fvm_r9p_poll`](../../../include/flashvm/r9p.h#L34) | Poll pending reads; PENDING preserves the output/count. [doc/RESOURCE_9P.md](../../../doc/RESOURCE_9P.md). |
| [`fvm_r9p_destroy`](../../../include/flashvm/r9p.h#L39) | Clunk owned fids before freeing server; FS remains host-owned. [doc/RESOURCE_9P.md](../../../doc/RESOURCE_9P.md). |
| [`fvm_rfs_destroy`](../../../include/flashvm/rfs.h#L82) | Free FS after the server ends every descriptor borrow. [doc/RESOURCE_FILES.md](../../../doc/RESOURCE_FILES.md). |
| [`fvm_rdispatch_destroy`](../../../include/flashvm/rdispatch.h#L131) | Free quiesced dispatcher after all adapter borrows end. [doc/RESOURCE_DISPATCH.md](../../../doc/RESOURCE_DISPATCH.md). |

[main.c](main.c) · [catalog](../../README.md#example-embedding-resource_9p)

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_r9p_create` | function | [include/flashvm/r9p.h:20](../../../include/flashvm/r9p.h#L20) |
| `fvm_r9p_destroy` | function | [include/flashvm/r9p.h:39](../../../include/flashvm/r9p.h#L39) |
| `fvm_r9p_poll` | function | [include/flashvm/r9p.h:34](../../../include/flashvm/r9p.h#L34) |
| `fvm_r9p_request` | function | [include/flashvm/r9p.h:28](../../../include/flashvm/r9p.h#L28) |
| `fvm_rdispatch_create` | function | [include/flashvm/rdispatch.h:95](../../../include/flashvm/rdispatch.h#L95) |
| `fvm_rdispatch_destroy` | function | [include/flashvm/rdispatch.h:131](../../../include/flashvm/rdispatch.h#L131) |
| `fvm_rdispatch_grant` | function | [include/flashvm/rdispatch.h:108](../../../include/flashvm/rdispatch.h#L108) |
| `fvm_rdispatch_register` | function | [include/flashvm/rdispatch.h:99](../../../include/flashvm/rdispatch.h#L99) |
| `fvm_rfs_create` | function | [include/flashvm/rfs.h:49](../../../include/flashvm/rfs.h#L49) |
| `fvm_rfs_destroy` | function | [include/flashvm/rfs.h:82](../../../include/flashvm/rfs.h#L82) |
<!-- source-links:end -->
