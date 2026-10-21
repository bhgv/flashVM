# Resource files

Runnable C89 tutorial for the file C API without VM, SDL, network or mount.
It demonstrates one-byte reads and explicit ownership and owner execution.

```sh
cmake -S flashVM -B build-files -DFVM_BUILD_EXAMPLES=ON
cmake --build build-files --target flashvm_examples_embedding_resource_files_main
./build-files/flashvm_examples_embedding_resource_files_main
```

Inside the component use `cmake -S .`. Requires C89 and CMake; the target
links only flashvm_srecord. Expected output: `resource files: hello`.
The FS borrows the dispatcher; the tutorial owns its file. Calls are serialized;
the callback runs only during pump. Error cleanup cancels queued work too. Close
the file before FS and FS before dispatcher. `1.1` is valid only in this fresh
instance; real applications enumerate names. One call per open; reopen for the
next call. This native echo is not a remote MovieClip implementation or OS mount.

| API | Ownership / execution |
| --- | --- |
| [`fvm_rdispatch_create`](../../../include/flashvm/rdispatch.h#L95) | Own one native dispatcher with single-owner policy. [doc/RESOURCE_DISPATCH.md](../../../doc/RESOURCE_DISPATCH.md). |
| [`fvm_rdispatch_register`](../../../include/flashvm/rdispatch.h#L99) | Copy registration; borrow callback userdata until unregister/destroy. [doc/RESOURCE_DISPATCH.md](../../../doc/RESOURCE_DISPATCH.md). |
| [`fvm_rdispatch_grant`](../../../include/flashvm/rdispatch.h#L108) | Owner grants explicit session READ authority. [doc/RESOURCE_DISPATCH.md](../../../doc/RESOURCE_DISPATCH.md). |
| [`fvm_rfs_create`](../../../include/flashvm/rfs.h#L49) | Own fixed FS storage; borrow dispatcher with exclusive session IDs. [doc/RESOURCE_FILES.md](../../../doc/RESOURCE_FILES.md). |
| [`fvm_rfs_walk`](../../../include/flashvm/rfs.h#L54) | Copy one checked child entry without OS traversal. [doc/RESOURCE_FILES.md](../../../doc/RESOURCE_FILES.md). |
| [`fvm_rfs_open`](../../../include/flashvm/rfs.h#L58) | Own one operation descriptor requiring READ|WRITE. [doc/RESOURCE_FILES.md](../../../doc/RESOURCE_FILES.md). |
| [`fvm_rfs_write`](../../../include/flashvm/rfs.h#L73) | Queue one copied request without executing a callback. [doc/RESOURCE_FILES.md](../../../doc/RESOURCE_FILES.md). |
| [`fvm_rdispatch_pump`](../../../include/flashvm/rdispatch.h#L119) | Owner explicitly executes at most one queued callback. [doc/RESOURCE_DISPATCH.md](../../../doc/RESOURCE_DISPATCH.md). |
| [`fvm_rfs_read`](../../../include/flashvm/rfs.h#L67) | Copy one stream byte and retain the remaining native response. [doc/RESOURCE_FILES.md](../../../doc/RESOURCE_FILES.md). |
| [`fvm_sbindings_init`](../../../include/flashvm/sbindings.h#L33) | Borrow one native slot for a file-local RX binding. [doc/SRECORD.md](../../../doc/SRECORD.md). |
| [`fvm_sbindings_apply`](../../../include/flashvm/sbindings.h#L49) | Verify declaration CRC before committing the RX association. [doc/SRECORD.md](../../../doc/SRECORD.md). |
| [`fvm_rreply_record_read`](../../../include/flashvm/rwire.h#L40) | Verify typed reply and copy counted result bytes. [doc/SRECORD.md](../../../doc/SRECORD.md). |
| [`fvm_rfs_close`](../../../include/flashvm/rfs.h#L79) | Cancel queued work or release the one open descriptor. [doc/RESOURCE_FILES.md](../../../doc/RESOURCE_FILES.md). |
| [`fvm_rfs_destroy`](../../../include/flashvm/rfs.h#L82) | Free FS after every file closes; dispatcher remains borrowed until success. [doc/RESOURCE_FILES.md](../../../doc/RESOURCE_FILES.md). |
| [`fvm_rdispatch_destroy`](../../../include/flashvm/rdispatch.h#L131) | Free quiesced dispatcher; refuse outstanding unacknowledged jobs. [doc/RESOURCE_DISPATCH.md](../../../doc/RESOURCE_DISPATCH.md). |

[main.c](main.c) · [catalog](../../README.md#example-embedding-resource_files)

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_rdispatch_create` | function | [include/flashvm/rdispatch.h:95](../../../include/flashvm/rdispatch.h#L95) |
| `fvm_rdispatch_destroy` | function | [include/flashvm/rdispatch.h:131](../../../include/flashvm/rdispatch.h#L131) |
| `fvm_rdispatch_grant` | function | [include/flashvm/rdispatch.h:108](../../../include/flashvm/rdispatch.h#L108) |
| `fvm_rdispatch_pump` | function | [include/flashvm/rdispatch.h:119](../../../include/flashvm/rdispatch.h#L119) |
| `fvm_rdispatch_register` | function | [include/flashvm/rdispatch.h:99](../../../include/flashvm/rdispatch.h#L99) |
| `fvm_rfs_close` | function | [include/flashvm/rfs.h:79](../../../include/flashvm/rfs.h#L79) |
| `fvm_rfs_create` | function | [include/flashvm/rfs.h:49](../../../include/flashvm/rfs.h#L49) |
| `fvm_rfs_destroy` | function | [include/flashvm/rfs.h:82](../../../include/flashvm/rfs.h#L82) |
| `fvm_rfs_open` | function | [include/flashvm/rfs.h:58](../../../include/flashvm/rfs.h#L58) |
| `fvm_rfs_read` | function | [include/flashvm/rfs.h:67](../../../include/flashvm/rfs.h#L67) |
| `fvm_rfs_walk` | function | [include/flashvm/rfs.h:54](../../../include/flashvm/rfs.h#L54) |
| `fvm_rfs_write` | function | [include/flashvm/rfs.h:73](../../../include/flashvm/rfs.h#L73) |
| `fvm_rreply_record_read` | function | [include/flashvm/rwire.h:40](../../../include/flashvm/rwire.h#L40) |
| `fvm_sbindings_apply` | function | [include/flashvm/sbindings.h:49](../../../include/flashvm/sbindings.h#L49) |
| `fvm_sbindings_init` | function | [include/flashvm/sbindings.h:33](../../../include/flashvm/sbindings.h#L33) |
<!-- source-links:end -->
