# Import an SVG rectangle

[Русский](SVG.ru.md) · [Resources](ASSETS.md)

`swl link OUTPUT.swf --svg NAME FILE` registers the rectangle from `FILE`
under the exported asset name `NAME`. Add `--place NAME X Y DEPTH` after the
resource to place it on the stage. Existing SWO and SWA inputs can be linked
into the same SWF.

```sh
swl link panel.swf --svg panel linker/examples/panel.svg --place panel 20 30 1
```

The input is an SVG document of up to 8 KiB. It contains a single
`<rect>` element in an `<svg>` root with namespace
`http://www.w3.org/2000/svg`. Both elements give the same width and height in
whole pixels from 1 to 3276, optionally followed by `px`. The rect's `x` and
`y` may be omitted or zero. Its fill is black by default. The `fill` attribute
admits `#RGB` and `#RRGGBB`. Optional `fill-opacity` is decimal from 0 to 1
with up to six fractional digits. The linker rounds it to an 8-bit alpha
channel. A UTF-8 BOM, XML 1.0 declaration with optional `encoding="UTF-8"`,
and XML comments are accepted. Other elements, attributes, entity references
and declarations are rejected.

## C API

```c
#include "swl.h"

int swl_add_svg(SwlContext *context, const char *name, const void *data,
                size_t size, char *error, size_t capacity);
```

`context` is the caller-owned linker context. `name` is a new resource name.
`data` and `size` supply borrowed XML bytes. `error` and `capacity` supply an
optional diagnostic buffer, with `NULL` and zero allowed together.
The synchronous call returns 1 after adding the encoded `DefineShape3` and 0
on error. On failure the context still holds its existing resources. The caller
can release the original bytes on return and must serialize calls that mutate
one context. [Declaration](include/swl.h) · [Implementation](src/svg_rect.c).
