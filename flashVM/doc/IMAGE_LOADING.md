# Atomic standalone images in MovieClip

[Русский](IMAGE_LOADING.ru.md) · [Sources](PLAYER_SOURCES.md) ·
[Unloading](MOVIE_UNLOAD.md) · [Roadmap](ROADMAP_STATUS.md).

## C API since v0.700

`fvm_movieclip_replace_image(context, clip, bytes, size, source, source_size)`
decodes a complete standalone PNG, static full-canvas GIF89a, or RGB/grayscale
JPEG into straight RGBA BitmapData and a one-frame stopped rectangle. It preserves
the same clip instance, prototype, placement and own properties. Prior children
are detached with retained provenance; matching child-name bindings are removed.
Retained clip handlers and lockroot survive. Relative resources use the copied
new source string. No user constructor, getter, setter or frame action runs.

Use the VM owner thread at a safe point outside active AVM1/native callbacks,
sound delivery, GC and teardown. Retain the context/target throughout the call;
pin objects retained across later collecting calls. Bytes/source are borrowed,
immutable through return, and never freed or modified. The private temporary
bitmap/drawing graph is protected by a native GC barrier. All validation, decoding
and graph allocations precede publication. Errors preserve the old target graph,
child bindings and drawing. Successful replacement requests a stage update.

PNG/GIF89a and progressive JPEG require a source SWF8 profile; baseline JPEG
requires SWF6. Native C-created clips with no SWF profile accept supported images.
The old JPEG decoder's embedded-resource contract is unchanged. Image detection
uses binary signatures, not filename extensions or HTTP MIME. Unknown formats,
unsupported profiles/codec builds, GIF87a, animated/partial-canvas GIF and unsupported
JPEG color spaces return [`FVM_UNSUPPORTED`](../include/flashvm/flashvm.h#L46). Recognized malformed streams/invalid
arguments return [`FVM_INVALID_ARGUMENT`](../include/flashvm/flashvm.h#L45); allocation failure returns
[`FVM_OUT_OF_MEMORY`](../include/flashvm/flashvm.h#L44). Other implementation failures become [`FVM_ERROR`](../include/flashvm/flashvm.h#L43).

Encoded bytes and each decoded RGBA payload are bounded by [`FVM_SWF_DEFAULT_LIMIT`](../include/flashvm/swf.h#L15);
codec dimensions are at most 2880 × 2880. Source names are nonempty counted text,
no embedded NUL, at most [`FVM_MOVIE_SOURCE_MAX_BYTES`](../include/flashvm/movieclip.h#L95). Temporary decode/vector/bitmap
copies can make peak memory larger than one payload; this is not a process memory
quota. C decoder allocations are released on C++ allocation failure as well.

[`fvm_movieclip_replace_resource`](../include/flashvm/movieclip.h#L129) selects FWS/CWS versus supported image bytes and
uses the corresponding atomic operation. Invalid SWF never falls back to image
decoding. The existing [`fvm_movieclip_replace_swf`](../include/flashvm/movieclip.h#L114) remains SWF-only. These additive
functions do not change any existing public descriptor or callback structure.
The desktop loader integration is documented separately as it becomes available.

## Build and tests

Image dependencies are bundled static libpng/giflib/libjpeg plus their existing
project support libraries. Use the independent component's image profile:

```sh
cmake -S flashVM -B build-images \
  -DFVM_DEFS_FILE="$PWD/flashVM/cmake/profiles/player-images.defs" \
  -DFVM_BUILD_TESTS=ON
cmake --build build-images --target flashvm_tests_image_codecs_movieclip_loading flashvm_tests_image_codecs_movieclip_loading_allocation
ctest --test-dir build-images -R 'image_codecs_movieclip_loading' --output-on-failure
```

The API does not require SDL, networking or an external AS compiler. A build whose
definitions select NO_IMAGE_CODECS retains explicit unsupported behavior. Do not
combine a NO_IMAGE_CODECS definitions file with an IMAGE_CODECS CMake override;
select the matching definitions file instead. The profile uses JIT; an unsupported
CPU retains the project's normal JIT availability checks.

Tests compare PNG/GIF transparency and JPEG pixels, retain identity and own data,
check detached origins/GC, malformed input, animated GIF, SWF/image dispatch and
SWF7 baseline/progressive profile distinctions. The allocation fixture walks C++
new/new[] failure boundaries and verifies unchanged old graph/bindings. It does
not replace the C decoders' separate malformed-input and allocator tests.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_ERROR` | enum constant | [include/flashvm/flashvm.h:43](../include/flashvm/flashvm.h#L43) |
| `FVM_INVALID_ARGUMENT` | enum constant | [include/flashvm/flashvm.h:45](../include/flashvm/flashvm.h#L45) |
| `FVM_MOVIE_SOURCE_MAX_BYTES` | macro / define | [include/flashvm/movieclip.h:95](../include/flashvm/movieclip.h#L95) |
| `fvm_movieclip_replace_image` | function | [include/flashvm/movieclip.h:125](../include/flashvm/movieclip.h#L125) |
| `fvm_movieclip_replace_resource` | function | [include/flashvm/movieclip.h:129](../include/flashvm/movieclip.h#L129) |
| `fvm_movieclip_replace_swf` | function | [include/flashvm/movieclip.h:114](../include/flashvm/movieclip.h#L114) |
| `FVM_OUT_OF_MEMORY` | enum constant | [include/flashvm/flashvm.h:44](../include/flashvm/flashvm.h#L44) |
| `FVM_SWF_DEFAULT_LIMIT` | macro / define | [include/flashvm/swf.h:15](../include/flashvm/swf.h#L15) |
| `FVM_UNSUPPORTED` | enum constant | [include/flashvm/flashvm.h:46](../include/flashvm/flashvm.h#L46) |
<!-- source-links:end -->
