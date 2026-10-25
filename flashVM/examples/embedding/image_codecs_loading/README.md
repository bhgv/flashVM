# Standalone image replacement from C89

[Русский](README.ru.md) · [Catalog](../../README.md#example-embedding-image_codecs_loading) ·
[API contract](../../../doc/IMAGE_LOADING.md).

[main.c](main.c) loads immutable counted PNG bytes into a native MovieClip through
both image and generic resource APIs, collects, renders into caller-owned RGBA,
checks an opaque red pixel, unloads and checks a retained own property. It uses no
SDL, filesystem input, network, SWF or AS compiler. [picture.inc](picture.inc)
contains the input; [make_picture.py](make_picture.py) regenerates it using stdlib.
Colors: opaque red, half-alpha green, opaque blue, transparent.

From repository root:

```sh
cmake -S flashVM -B build-image-core -DFVM_BUILD_EXAMPLES=ON \
  -DFVM_DEFS_FILE="$PWD/flashVM/cmake/profiles/player-images.defs"
cmake --build build-image-core --target flashvm_examples_embedding_image_codecs_loading_main
./build-image-core/flashvm_examples_embedding_image_codecs_loading_main
python3 flashVM/examples/embedding/image_codecs_loading/make_picture.py
```

Expected exit 0 and `image pixels verified; instance data retained`. Core-only
component builds can use `-S .` inside flashVM with the corresponding profile path.
Dependencies are the bundled static codecs. For another OS/CPU use the documented
[target toolchain](../../../doc/DESKTOP_FRONTEND.md) and build the same target;
this example itself needs no SDL target SDK. Windows appends `.exe`. Cross-linking
does not prove execution on that processor. NO_IMAGE_CODECS excludes this example.

All VM calls use the owner thread. The host pin retains the instance across GC;
context destruction releases it before runtime teardown. PNG input is immutable,
never freed by the VM; bitmap pixels are copied/owned by the managed graph. Output
RGBA is borrowed only during render. Errors use ordinary diagnostics/cleanup.
Dimensions, profiles, unsupported GIF variants, errors and allocation limits are
specified in IMAGE_LOADING.md. This small example does not certify every codec.

| API | Purpose and ownership; guide |
| --- | --- |
| [`fvm_movieclip_content_info_init`](../../../include/flashvm/movieclip.h#L72) | Initialize caller-owned snapshot and exact ABI size without allocation; [MOVIECLIP_CONTENT_INFO.md](../../../doc/MOVIECLIP_CONTENT_INFO.md). |
| [`fvm_movieclip_get_content_info`](../../../include/flashvm/movieclip.h#L83) | Copy committed source/pixel scalars without script or I/O; errors preserve the descriptor; [MOVIECLIP_CONTENT_INFO.md](../../../doc/MOVIECLIP_CONTENT_INFO.md). |
| [`fvm_config_init`](../../../include/flashvm/flashvm.h#L70) | Initialize caller configuration before runtime creation; API_INDEX.md. |
| [`fvm_runtime_create`](../../../include/flashvm/flashvm.h#L74) | Create enclosing owner, released last; API_INDEX.md. |
| [`fvm_context_create`](../../../include/flashvm/flashvm.h#L82) | Create managed object owner; API_INDEX.md. |
| [`fvm_standard_install`](../../../include/flashvm/standard.h#L22) | Install trusted native classes; API_INDEX.md. |
| [`fvm_movieclip_create`](../../../include/flashvm/movieclip.h#L120) | Return borrowed native instance; MOVIECLIP_TICKS.md. |
| [`fvm_gc_pin`](../../../include/flashvm/gc.h#L60) | Retain graph across later calls; GC.md. |
| [`fvm_value_string_size`](../../../include/flashvm/value.h#L67) | Read counted borrowed source length without allocation or ownership transfer; API_INDEX.md. |
| [`fvm_value_number`](../../../include/flashvm/value.h#L50) | Initialize a scalar with no managed storage; API_INDEX.md. |
| [`fvm_object_set`](../../../include/flashvm/value.h#L106) | Copy own data or invoke a named dimension setter synchronously; API_INDEX.md. |
| [`fvm_movieclip_replace_image`](../../../include/flashvm/movieclip.h#L235) | Borrow image/source during atomic owner-safe replacement; IMAGE_LOADING.md. |
| [`fvm_movieclip_replace_resource`](../../../include/flashvm/movieclip.h#L241) | Select SWF/image by bytes with the same transaction; IMAGE_LOADING.md. |
| [`fvm_gc_collect`](../../../include/flashvm/gc.h#L54) | Collect while pin retains the clip; GC.md. |
| [`fvm_movieclip_render`](../../../include/flashvm/movieclip.h#L174) | Borrow caller RGBA during synchronous native rendering; IMAGE_LOADING.md. |
| [`fvm_movieclip_unload`](../../../include/flashvm/movieclip.h#L264) | Clear content, retain identity/data; MOVIE_UNLOAD.md. |
| [`fvm_object_get`](../../../include/flashvm/value.h#L102) | Read scalars or a borrowed managed source string; API_INDEX.md. |
| [`fvm_context_destroy`](../../../include/flashvm/flashvm.h#L86) | Release graph/pin before runtime; API_INDEX.md. |
| [`fvm_runtime_destroy`](../../../include/flashvm/flashvm.h#L78) | Release enclosing runtime after context; API_INDEX.md. |


The C89 example also reads and resizes `_width` through [fvm_object_get](../../../include/flashvm/value.h#L102)/set,
restores the original geometry before checking pixels, and reads the counted
`_url` source. See [named properties](../../../doc/MOVIECLIP_NAMED_PROPERTIES.md)
for conversion, callback lifetime and transport behavior.


Since v0.709 the example initializes [FVM_MovieClipContentInfo](../../../include/flashvm/movieclip.h#L59), checks PNG input
length versus original RGBA length/dimensions, and requires an explicit EMPTY
record after unload. The snapshot contains no borrowed pointers, so it can be
retained independently of the graph. This does not measure live transfer progress.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_config_init` | function | [include/flashvm/flashvm.h:70](../../../include/flashvm/flashvm.h#L70) |
| `fvm_context_create` | function | [include/flashvm/flashvm.h:82](../../../include/flashvm/flashvm.h#L82) |
| `fvm_context_destroy` | function | [include/flashvm/flashvm.h:86](../../../include/flashvm/flashvm.h#L86) |
| `fvm_gc_collect` | function | [include/flashvm/gc.h:54](../../../include/flashvm/gc.h#L54) |
| `fvm_gc_pin` | function | [include/flashvm/gc.h:60](../../../include/flashvm/gc.h#L60) |
| `fvm_movieclip_content_info_init` | function | [include/flashvm/movieclip.h:72](../../../include/flashvm/movieclip.h#L72) |
| `fvm_movieclip_create` | function | [include/flashvm/movieclip.h:120](../../../include/flashvm/movieclip.h#L120) |
| `fvm_movieclip_get_content_info` | function | [include/flashvm/movieclip.h:83](../../../include/flashvm/movieclip.h#L83) |
| `fvm_movieclip_render` | function | [include/flashvm/movieclip.h:174](../../../include/flashvm/movieclip.h#L174) |
| `fvm_movieclip_replace_image` | function | [include/flashvm/movieclip.h:235](../../../include/flashvm/movieclip.h#L235) |
| `fvm_movieclip_replace_resource` | function | [include/flashvm/movieclip.h:241](../../../include/flashvm/movieclip.h#L241) |
| `fvm_movieclip_unload` | function | [include/flashvm/movieclip.h:264](../../../include/flashvm/movieclip.h#L264) |
| `FVM_MovieClipContentInfo` | struct | [include/flashvm/movieclip.h:59](../../../include/flashvm/movieclip.h#L59) |
| `fvm_object_get` | function | [include/flashvm/value.h:102](../../../include/flashvm/value.h#L102) |
| `fvm_object_set` | function | [include/flashvm/value.h:106](../../../include/flashvm/value.h#L106) |
| `fvm_runtime_create` | function | [include/flashvm/flashvm.h:74](../../../include/flashvm/flashvm.h#L74) |
| `fvm_runtime_destroy` | function | [include/flashvm/flashvm.h:78](../../../include/flashvm/flashvm.h#L78) |
| `fvm_standard_install` | function | [include/flashvm/standard.h:22](../../../include/flashvm/standard.h#L22) |
| `fvm_value_number` | function | [include/flashvm/value.h:50](../../../include/flashvm/value.h#L50) |
| `fvm_value_string_size` | function | [include/flashvm/value.h:67](../../../include/flashvm/value.h#L67) |
<!-- source-links:end -->
