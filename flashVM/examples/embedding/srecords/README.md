# Binary exchange with independent directional schemas

[Русский](README.ru.md) · [Catalog](../../README.md#example-embedding-srecords) · [Contract](../../../doc/SRECORD.md).

This executable C89 tutorial needs no VM or SDL. It demonstrates metadata/value
records, one-byte receive fragments and ordinary fwrite/fread. A→B declares tag 3
as ieee754.f64.be; B→A declares tag 3 as boolean. These model independent directions.
The optional file concatenates their transcripts for demonstration only; a real
multiplexed transport needs an explicit direction envelope, not an implicit change
to the S-record layout.

## Build / Сборка

```sh
cmake -S flashVM -B build-srecord -DFVM_BUILD_TESTS=ON -DFVM_BUILD_EXAMPLES=ON \
  -DFVM_DEFS_FILE="$PWD/flashVM/cmake/profiles/minimal.defs"
cmake --build build-srecord --target flashvm_examples_embedding_srecords_main \
  flashvm_srecord_oracle flashvm_srecord_cpp98 flashvm_srecord_fuzz --parallel
./build-srecord/flashvm_examples_embedding_srecords_main
./build-srecord/flashvm_examples_embedding_srecords_main transcript.bin
ctest --test-dir build-srecord -R '^flashvm_srecord_' --output-on-failure
```

A C89/C++98 toolchain and CMake are required; Python 3 is for independent tests,
not for the C executable. From a detached VM component use -S . and its own
absolute cmake/profiles/minimal.defs path. Preserve the shared MIT LICENSE.

## Expected / Ожидается

```text
tag=3 type=ieee754.f64.be bytes=8
tag=3 type=boolean bytes=1
```

Exit 0 means both directions, and the optional file write/read, verified. Exit 1
means protocol/I/O failure; 2 means invalid arguments. The explicit file is created/
overwritten and contains NULs, not text. An output failure may leave a partial file.
Every FILE is closed. Paths follow native CRT rules; no Windows UTF-16 path adapter
is added here.

Float64 1.25 is supplied as exact big-endian bytes 3f f4 00 00 00 00 00 00, never
native-double memcpy. Boolean true uses canonical integer 81. These example
schema identities describe payload; the codec does not execute schema handlers.

## Ownership and limits

All buffers, registries and metadata are stack-owned by the host. Inputs/outputs
are disjoint. Apply copies identities, view borrows receive until reset, lookup
copies metadata. [GC:] No VM runtime, object or pin is created. No locks are needed
in this local example; shared use would require host synchronization.

This is an ordinary file of binary records, not a mounted remote filesystem.
Graph cycles/shared references, resource dispatch, sockets, authority and Inferno
mount/bind are not implemented here. Accepted plan: [namespace](../../../doc/RESOURCE_NAMESPACE.md).

## C API

| API | Purpose and ownership |
| --- | --- |
| [`fvm_sbindings_init`](../../../include/flashvm/sbindings.h#L33) | Create an empty incoming table in caller slots, without allocation. [EN](../../../doc/SRECORD.md) / [RU](../../../doc/SRECORD.ru.md). |
| [`fvm_sbindings_payload_write`](../../../include/flashvm/sbindings.h#L43) | Encode the full immutable declaration list to a disjoint caller payload buffer; atomic validation. [EN](../../../doc/SRECORD.md) / [RU](../../../doc/SRECORD.ru.md). |
| [`fvm_srecord_write`](../../../include/flashvm/srecord.h#L66) | Frame metadata/value bytes with canonical length/tag and checksum; borrow source only during call. [EN](../../../doc/SRECORD.md) / [RU](../../../doc/SRECORD.ru.md). |
| [`fvm_sbindings_apply`](../../../include/flashvm/sbindings.h#L49) | Verify tag-zero frame and copy all bindings atomically; conflicts leave the table untouched. [EN](../../../doc/SRECORD.md) / [RU](../../../doc/SRECORD.ru.md). |
| [`fvm_sstream_init`](../../../include/flashvm/sstream.h#L24) | Bind native state to owned receive buffer and explicit record quota; no I/O. [EN](../../../doc/SRECORD.md) / [RU](../../../doc/SRECORD.ru.md). |
| [`fvm_sstream_push`](../../../include/flashvm/sstream.h#L33) | Retain each fragment; only OK publishes a borrowed verified view. MORE still reports consumed bytes. [EN](../../../doc/SRECORD.md) / [RU](../../../doc/SRECORD.ru.md). |
| [`fvm_sbindings_lookup`](../../../include/flashvm/sbindings.h#L37) | Copy the receiving direction's type/version metadata; no global tag interpretation. [EN](../../../doc/SRECORD.md) / [RU](../../../doc/SRECORD.ru.md). |
| [`fvm_sstream_reset`](../../../include/flashvm/sstream.h#L38) | Invalidate the last view after its last use; retain caller storage, discard state. [EN](../../../doc/SRECORD.md) / [RU](../../../doc/SRECORD.ru.md). |
| [`fvm_srecord_read`](../../../include/flashvm/srecord.h#L80) | Verify one complete record in fread bytes and return its exact consumed extent; view borrows input. [EN](../../../doc/SRECORD.md) / [RU](../../../doc/SRECORD.ru.md). |

## Cross / Другие OS и CPU

```sh
cmake -S flashVM -B build-srecord-target \
  -DCMAKE_TOOLCHAIN_FILE=/path/to/target-toolchain.cmake \
  -DFVM_BUILD_TESTS=OFF -DFVM_BUILD_EXAMPLES=ON \
  -DFVM_DEFS_FILE="$PWD/flashVM/cmake/profiles/minimal.defs"
cmake --build build-srecord-target --target flashvm_examples_embedding_srecords_main --parallel
```

Use target-matching headers/libc/linker. Protocol bytes and 32-bit limbs are
explicit, independent of native unsigned-long width or byte order. ARM64/RISC-V64/
Windows x64 cross-link evidence is not execution on those devices. Native OS,
disconnect, filesystem permissions and future adapters need separate acceptance.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_sbindings_apply` | function | [include/flashvm/sbindings.h:49](../../../include/flashvm/sbindings.h#L49) |
| `fvm_sbindings_init` | function | [include/flashvm/sbindings.h:33](../../../include/flashvm/sbindings.h#L33) |
| `fvm_sbindings_lookup` | function | [include/flashvm/sbindings.h:37](../../../include/flashvm/sbindings.h#L37) |
| `fvm_sbindings_payload_write` | function | [include/flashvm/sbindings.h:43](../../../include/flashvm/sbindings.h#L43) |
| `fvm_srecord_read` | function | [include/flashvm/srecord.h:80](../../../include/flashvm/srecord.h#L80) |
| `fvm_srecord_write` | function | [include/flashvm/srecord.h:66](../../../include/flashvm/srecord.h#L66) |
| `fvm_sstream_init` | function | [include/flashvm/sstream.h:24](../../../include/flashvm/sstream.h#L24) |
| `fvm_sstream_push` | function | [include/flashvm/sstream.h:33](../../../include/flashvm/sstream.h#L33) |
| `fvm_sstream_reset` | function | [include/flashvm/sstream.h:38](../../../include/flashvm/sstream.h#L38) |
<!-- source-links:end -->
