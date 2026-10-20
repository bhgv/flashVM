# Committed MovieClip content information

[Русский](MOVIECLIP_CONTENT_INFO.ru.md) · [Images](IMAGE_LOADING.md) ·
[Named properties](MOVIECLIP_NAMED_PROPERTIES.md) · [Roadmap](ROADMAP_STATUS.md).

## C interface since v0.707

`fvm_movieclip_content_info_init(out)` initializes caller-owned
[`FVM_MovieClipContentInfo`](../include/flashvm/movieclip.h#L46) including its exact `struct_size`, EMPTY format and zero
fields. Null is INVALID_ARGUMENT. It allocates nothing and needs no context.

`fvm_movieclip_get_content_info(context, clip, out)` copies scalar metadata of the
last successfully imported external SWF/image or explicit unload. Initialize the
required output first. The clip must be a native instance owned by the context;
call on its owner thread outside collection/teardown and retain it throughout.
The query does not call script, decode data, perform transport or retain inputs.
No object pointer or borrowed string is returned; the successful snapshot remains
valid after replacement, collection or destruction of the clip/context.

| Field | Meaning |
| --- | --- |
| `format` | SWF, PNG, supported static GIF, JPEG, or explicit EMPTY after unload. |
| `swf_version` | Own imported SWF header version; zero for images/EMPTY. This differs from the source language profile retained for interpreting later operations. |
| `encoded_bytes` | Exact input length including the SWF/image headers. |
| `decoded_bytes` | Complete canonical uncompressed SWF length, or original straight RGBA image payload length. |
| `pixel_width`, `pixel_height` | Original decoded image dimensions, independent of current `_width`/`_height`, scaling, rotation or later drawing. Zero for SWF/EMPTY. |

Counters fit [`FVM_MOVIECLIP_CONTENT_MAX_BYTES`](../include/flashvm/movieclip.h#L44); current image limits remain those
in the linked codec guide. RGBA payload size is width × height × four channels.
These fields describe source payloads, not process memory or a live bitmap's
current allocation. Duplicate imported clips retain the immutable source record;
unloading one duplicate does not change the other's metadata.

NOT_FOUND means the graph has no imported-content record: native/dynamically drawn
clips, internal SWF symbols or graphs created by older releases. It does not mean
that their rendered geometry is empty. Explicit unload instead returns OK with
EMPTY and all other fields zero. The class prototype, foreign context, invalid ABI
size or corrupt record returns INVALID_ARGUMENT. Allocation failure returns
OUT_OF_MEMORY. Every failure preserves the entire caller descriptor.

Metadata is built on the private replacement graph before publication. Malformed
input, unsupported codecs/profiles and allocation failure retain the previous
content and record together. Caller input bytes can be released after import;
no input buffer address is stored. Raw record types and bounds are checked on
query, including graphs restored from serialization. Records and their owning
edges survive GC, duplicate and ordinary graph transport without native pointers.

This is completed-content accounting. Queued or incomplete transfers retain the
previous committed record until publication. Use the host load monitor for live
transfer disposition; these counters do not implement progressive decoding or a
full MovieClipLoader event lifecycle.

## Build and verification

```sh
cmake -S flashVM -B build-content
cmake --build build-content --target flashvm_tests_movieclip_content_info
ctest --test-dir build-content -R '^flashvm_tests_movieclip_content_info$' --output-on-failure
```

The core test needs no SDL/network/image codecs. It checks FWS/CWS input versus
canonical size, copied input ownership, unchanged failure outputs, malformed
replacement, serialization, collection and explicit unload. The optional
`image_codecs_movieclip_content_info` fixture covers PNG/GIF/JPEG source bytes,
original pixels after named resizing, failed import and duplicated metadata.
Existing SWF/image allocation sweeps also traverse record preparation allocations.
See the linked C and player image tutorials for native/cross profile setup.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_movieclip_content_info_init` | function | [include/flashvm/movieclip.h:57](../include/flashvm/movieclip.h#L57) |
| `FVM_MOVIECLIP_CONTENT_MAX_BYTES` | macro / define | [include/flashvm/movieclip.h:44](../include/flashvm/movieclip.h#L44) |
| `fvm_movieclip_get_content_info` | function | [include/flashvm/movieclip.h:66](../include/flashvm/movieclip.h#L66) |
| `FVM_MovieClipContentInfo` | struct | [include/flashvm/movieclip.h:46](../include/flashvm/movieclip.h#L46) |
<!-- source-links:end -->
