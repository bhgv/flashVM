# AGG portable core

Source repository: https://github.com/ghaerr/agg-2.6
Pinned commit: `c4f36b4432142f22c0bf82c6fbdb41567a236be2`.
Included: `agg-src/include`, top-level `agg-src/src/*.cpp`, COPYING material
(`upstream/copying`), README and ChangeLog. The FreeType bridge is included with FONTS; GUI/platform adapters and
examples are excluded; FlashVM system adapters remain in `ports/`.
The imported core identifies itself as AGG 2.4 and retains its permissive
copyright notices and dual-license text. Repository name is not a version claim.

Marked FlashVM patches:
- `agg_config.h`: standard `long`/`unsigned long` instead of non-C++98
  `long long`. **Only RGBA8 is supported. 16-bit component paths are unsafe
  with this configuration and must not be used.**
- `agg_rasterizer_cells_aa.h`: standard `long` arithmetic. The public adapter
  bounds coordinates to +/-4096 and dimensions to 4096; its 32-bit arithmetic
  is bounded. Return after recursive splitting avoids drawing the original
  segment a second time.
- `agg_pixfmt_rgba.h`: trailing enum commas removed for strict C++98.

The adapter clips, blends straight RGBA8 and fills one nonzero-winding polygon.
It is not a SWF shape decoder, complete renderer or filter implementation.
Integer alpha blending can differ by one RGB unit after rounding.

Ядро AGG закреплено на указанном коммите; лицензии сохранены. Адаптер поддерживает
только RGBA8, размеры до 4096 и координаты +/-4096. Изменения для C++98 отмечены
в исходниках. 16-битные компоненты с данной конфигурацией использовать нельзя.
Это растеризатор полигонов, пока без декодера SWF-фигур и фильтров.

FONTS adds the pinned font_freetype bridge (the FT_Outline tag pointer was
adapted to FreeType 2.14.3). The independent FlashVM path API now also supports
Bezier curves, fill/stroke and image masks; see [fonts](../../doc/FONT_LIBRARIES.md).

Exception-safety corrections (FlashVM bitmap rendering):
- Publish raster-cell block counts only after allocation succeeds; free the pointer
  table even when the first data block fails.
- Allocate pod_array/pod_vector replacements before freeing their old storage.
- Free vertex pointer tables after a failed first vertex block, avoiding
  before-begin pointer arithmetic in reverse destruction loops.
These are narrow functional patches; upstream code has not been reformatted.
Regression coverage: movieclip_bitmap_allocation in AGG/Cairo and the dedicated
AGG sanitizer profile.
