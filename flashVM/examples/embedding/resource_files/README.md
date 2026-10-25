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
| [`fvm_rdispatch_create`](../../../include/flashvm/rdispatch.h#L109) | Own one native dispatcher with single-owner policy. [doc/RESOURCE_DISPATCH.md](../../../doc/RESOURCE_DISPATCH.md). |
| [`fvm_rdispatch_register`](../../../include/flashvm/rdispatch.h#L115) | Copy registration; borrow callback userdata until unregister/destroy. [doc/RESOURCE_DISPATCH.md](../../../doc/RESOURCE_DISPATCH.md). |
| [`fvm_rdispatch_grant`](../../../include/flashvm/rdispatch.h#L128) | Owner grants explicit session READ authority. [doc/RESOURCE_DISPATCH.md](../../../doc/RESOURCE_DISPATCH.md). |
| [`fvm_rfs_create`](../../../include/flashvm/rfs.h#L55) | Own fixed FS storage; borrow dispatcher with exclusive session IDs. [doc/RESOURCE_FILES.md](../../../doc/RESOURCE_FILES.md). |
| [`fvm_rfs_walk`](../../../include/flashvm/rfs.h#L64) | Copy one checked child entry without OS traversal. [doc/RESOURCE_FILES.md](../../../doc/RESOURCE_FILES.md). |
| [`fvm_rfs_open`](../../../include/flashvm/rfs.h#L70) | Own one operation descriptor requiring READ|WRITE. [doc/RESOURCE_FILES.md](../../../doc/RESOURCE_FILES.md). |
| [`fvm_rfs_write`](../../../include/flashvm/rfs.h#L91) | Queue one copied request without executing a callback. [doc/RESOURCE_FILES.md](../../../doc/RESOURCE_FILES.md). |
| [`fvm_rdispatch_pump`](../../../include/flashvm/rdispatch.h#L145) | Owner explicitly executes at most one queued callback. [doc/RESOURCE_DISPATCH.md](../../../doc/RESOURCE_DISPATCH.md). |
| [`fvm_rfs_read`](../../../include/flashvm/rfs.h#L83) | Copy one stream byte and retain the remaining native response. [doc/RESOURCE_FILES.md](../../../doc/RESOURCE_FILES.md). |
| [`fvm_sbindings_init`](../../../include/flashvm/sbindings.h#L37) | Borrow one native slot for a file-local RX binding. [doc/SRECORD.md](../../../doc/SRECORD.md). |
| [`fvm_sbindings_apply`](../../../include/flashvm/sbindings.h#L59) | Verify declaration CRC before committing the RX association. [doc/SRECORD.md](../../../doc/SRECORD.md). |
| [`fvm_rreply_record_read`](../../../include/flashvm/rwire.h#L48) | Verify typed reply and copy counted result bytes. [doc/SRECORD.md](../../../doc/SRECORD.md). |
| [`fvm_rfs_close`](../../../include/flashvm/rfs.h#L99) | Cancel queued work or release the one open descriptor. [doc/RESOURCE_FILES.md](../../../doc/RESOURCE_FILES.md). |
| [`fvm_rfs_destroy`](../../../include/flashvm/rfs.h#L104) | Free FS after every file closes; dispatcher remains borrowed until success. [doc/RESOURCE_FILES.md](../../../doc/RESOURCE_FILES.md). |
| [`fvm_rdispatch_destroy`](../../../include/flashvm/rdispatch.h#L163) | Free quiesced dispatcher; refuse outstanding unacknowledged jobs. [doc/RESOURCE_DISPATCH.md](../../../doc/RESOURCE_DISPATCH.md). |

[main.c](main.c) · [catalog](../../README.md#example-embedding-resource_files)

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_rdispatch_create` | function | [include/flashvm/rdispatch.h:109](../../../include/flashvm/rdispatch.h#L109) |
| `fvm_rdispatch_destroy` | function | [include/flashvm/rdispatch.h:163](../../../include/flashvm/rdispatch.h#L163) |
| `fvm_rdispatch_grant` | function | [include/flashvm/rdispatch.h:128](../../../include/flashvm/rdispatch.h#L128) |
| `fvm_rdispatch_pump` | function | [include/flashvm/rdispatch.h:145](../../../include/flashvm/rdispatch.h#L145) |
| `fvm_rdispatch_register` | function | [include/flashvm/rdispatch.h:115](../../../include/flashvm/rdispatch.h#L115) |
| `fvm_rfs_close` | function | [include/flashvm/rfs.h:99](../../../include/flashvm/rfs.h#L99) |
| `fvm_rfs_create` | function | [include/flashvm/rfs.h:55](../../../include/flashvm/rfs.h#L55) |
| `fvm_rfs_destroy` | function | [include/flashvm/rfs.h:104](../../../include/flashvm/rfs.h#L104) |
| `fvm_rfs_open` | function | [include/flashvm/rfs.h:70](../../../include/flashvm/rfs.h#L70) |
| `fvm_rfs_read` | function | [include/flashvm/rfs.h:83](../../../include/flashvm/rfs.h#L83) |
| `fvm_rfs_walk` | function | [include/flashvm/rfs.h:64](../../../include/flashvm/rfs.h#L64) |
| `fvm_rfs_write` | function | [include/flashvm/rfs.h:91](../../../include/flashvm/rfs.h#L91) |
| `fvm_rreply_record_read` | function | [include/flashvm/rwire.h:48](../../../include/flashvm/rwire.h#L48) |
| `fvm_sbindings_apply` | function | [include/flashvm/sbindings.h:59](../../../include/flashvm/sbindings.h#L59) |
| `fvm_sbindings_init` | function | [include/flashvm/sbindings.h:37](../../../include/flashvm/sbindings.h#L37) |
<!-- source-links:end -->
